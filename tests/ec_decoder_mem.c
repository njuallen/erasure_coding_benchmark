/*
 * Copyright (c) 2005 Topspin Communications.  All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "ec_common.h"
#include <ecOffload/eco_decoder.h>

struct decoder_context {
	struct ibv_context	*context;
	struct ibv_pd		*pd;
	struct ec_context	*ec_ctx;
};

struct thread_arg_t {
	struct inargs *in;
	struct decoder_context *ctx;
};

static struct decoder_context *
init_ctx(struct ibv_device *ib_dev, struct inargs *in)
{
	struct decoder_context *ctx;

	ctx = calloc(1, sizeof(*ctx));
	if (!ctx) {
		err_log("Failed to allocate encoder context\n");
		return NULL;
	}

	ctx->context = ibv_open_device(ib_dev);
	if (!ctx->context) {
		err_log("Couldn't get context for %s\n",
				ibv_get_device_name(ib_dev));
		goto free_ctx;
	}

	ctx->pd = ibv_alloc_pd(ctx->context);
	if (!ctx->pd) {
		err_log("Failed to allocate PD\n");
		goto close_device;
	}

	ctx->ec_ctx = alloc_ec_ctx(ctx->pd, in->frame_size,
			in->k, in->m, in->w, 1, in->failed_blocks);
	if (!ctx->ec_ctx) {
		err_log("Failed to allocate EC context\n");
		goto dealloc_pd;
	}

	return ctx;

dealloc_pd:
	ibv_dealloc_pd(ctx->pd);
close_device:
	ibv_close_device(ctx->context);
free_ctx:
	free(ctx);

	return NULL;
}

static void close_ctx(struct decoder_context *ctx)
{
	free_ec_ctx(ctx->ec_ctx);
	ibv_dealloc_pd(ctx->pd);

	if (ibv_close_device(ctx->context))
		err_log("Couldn't release context\n");

	free(ctx);
}

static void usage(const char *argv0)
{
	printf("Usage:\n");
	printf("  %s            start EC encoder\n", argv0);
	printf("\n");
	printf("Options:\n");
	printf("  -i, --ib-dev=<dev>           use IB device <dev> (default first device found)\n");
	printf("  -k, --data_blocks=<blocks>   Number of data blocks\n");
	printf("  -m, --code_blocks=<blocks>   Number of code blocks\n");
	printf("  -w, --gf=<gf>                Galois field GF(2^w)\n");
	printf("  -E, --erasures=<erasures>  Comma separated failed blocks\n");
	printf("  -F, --file_size=<size>       size of file in bytes\n");
	printf("  -s, --frame_size=<size>      size of EC frame\n");
	printf("  -t, --thread_number=<number> number of threads used\n");
	printf("  -V, --verbs                  use verbs api\n");
	printf("  -S, --software_ec            use software EC(Jerasure library)\n");
	printf("  -d, --debug                  print debug messages\n");
	printf("  -v, --verbose                add verbosity\n");
	printf("  -h, --help                   display this output\n");
}

static int process_inargs(int argc, char *argv[], struct inargs *in)
{
	int err;
	struct option long_options[] = {
		{ .name = "ib-dev",        .has_arg = 1, .val = 'i' },
		{ .name = "erasures",      .has_arg = 1, .val = 'E' },
		{ .name = "file_size",     .has_arg = 1, .val = 'F' },
		{ .name = "frame_size",    .has_arg = 1, .val = 's' },
		{ .name = "data_blocks",   .has_arg = 1, .val = 'k' },
		{ .name = "code_blocks",   .has_arg = 1, .val = 'm' },
		{ .name = "gf",            .has_arg = 1, .val = 'w' },
		{ .name = "thread_number", .has_arg = 1, .val = 't' },
		{ .name = "verbs",         .has_arg = 0, .val = 'V' },
		{ .name = "sw",            .has_arg = 0, .val = 'S' },
		{ .name = "debug",         .has_arg = 0, .val = 'd' },
		{ .name = "verbose",       .has_arg = 0, .val = 'v' },
		{ .name = "help",          .has_arg = 0, .val = 'h' },
		{ .name = 0, .has_arg = 0, .val = 0 }
	};

	err = common_process_inargs(argc, argv, "i:E:F:s:k:m:w:t:VShdv",
			long_options, in, usage);
	if (err)
		return err;

	return 0;
}

static void zero_erasures(struct ec_context *ctx, void *data_buf, void *code_buf)
{
	int i;

	for (i = 0; i < ctx->attr.k; i++)
		if (ctx->int_erasures[i])
			memset(data_buf + i * ctx->block_size, 0, ctx->block_size);

	for (i = 0; i < ctx->attr.m; i++) {
		if (ctx->int_erasures[i + ctx->attr.k])
			memset(code_buf + i * ctx->block_size, 0, ctx->block_size);
	}
}

/* the total number of decode operations needs to be done
 * all threads share this counter
 * each time, each thread gets some work and decreases this counter
 * If the value of the counter is below zero,
 * there is no more work, threads can exit.
 */
volatile long long iterations = 0;

static int decode_benchmark(struct decoder_context *ctx, 
		struct inargs *in)
{
	struct ec_context *ec_ctx = ctx->ec_ctx;
	int i;
	int err;

	while(1) {
		long long batch = 20;
		// use batching to avoid much contention
		long long iter = __sync_fetch_and_sub(&iterations, batch);
		// finished
		if(iter <= 0)
			break;

		// the number of operations to do this time
		int nops = (iter < batch) ? iter : batch;

		// uses verbs API
		if(in->verbs) {
			for(i = 0; i < nops; i++) {
				zero_erasures(ec_ctx, ec_ctx->data.buf, ec_ctx->code.buf);
				err = ibv_exp_ec_decode_sync(ec_ctx->calc, &ec_ctx->mem,
						ec_ctx->u8_erasures, ec_ctx->de_mat);
				if (err) {
					fprintf(stderr, "Failed ibv_exp_ec_decode (%d)\n", err);
					return err;
				}
			}
		}

		// uses Jerasure library
		if(in->sw) {
			for(i = 0; i < nops; i++) {
				memset(ec_ctx->code.buf, 0, ec_ctx->block_size * ec_ctx->attr.m);
				err = sw_ec_encode(ec_ctx);
				if (err) {
					err_log("Failed sw_ec_encode (%d)\n", err);
					return err;
				}
			}
		}

		/* mellanox said that this library is not thread safe, 
		 * so we can not use it here
		 */
	}
	return 0;
}

static void set_buffers(struct decoder_context *ctx)
{
	struct ec_context *ec_ctx = ctx->ec_ctx;
	int i;

	ec_ctx->data_arr = calloc(ec_ctx->attr.k, sizeof(*ec_ctx->data_arr));
	ec_ctx->code_arr = calloc(ec_ctx->attr.m, sizeof(*ec_ctx->code_arr));

	/* data.buf is already allocated and has been memory registered
	 * here we only let pointers in data_arr point to the correct location
	 * data_arr pointers works just like sge
	 * but sge is for verbs api usage and data_arr pointers is for our own usage
	 */
	for (i = 0; i < ec_ctx->attr.k ; i++) {
		ec_ctx->data_arr[i] = ec_ctx->data.buf + i * ec_ctx->block_size;
	}
	for (i = 0; i < ec_ctx->attr.m ; i++) {
		ec_ctx->code_arr[i] = ec_ctx->code.buf + i * ec_ctx->block_size;
	}
}


void *thread(void *vargp);

int main(int argc, char *argv[])
{
	struct ibv_device *device;
	struct inargs in;
	int err;

	err = process_inargs(argc, argv, &in);
	if (err)
		return err;

	device = find_device(in.devname);
	if (!device)
		return -EINVAL;

	int block_size = align_any((in.frame_size + in.k - 1) / in.k, 64);
	// the real frame_size we use after aligning block_size to 64 bytes
	int frame_size = block_size * in.k;
	iterations = in.file_size / frame_size;
	
	dbg_log("iterations: %lld\nblock_size: %d\nframe_size: %d\n", 
			iterations, block_size, frame_size);

	// generate the data blocks to decode
	uint8_t *data = (uint8_t *)malloc(sizeof(uint8_t) * frame_size);
	if(!data) {
		err_log("failed to allocate data blocks\n");
		return -ENOMEM;
	}
	int i = 0;
	srand(time(NULL));
	for(i = 0; i < frame_size; i++)
		data[i] = rand();

	int nthread = in.nthread;
	pthread_t *tid = (pthread_t *)malloc(nthread * sizeof(pthread_t)); 
	if (!tid) {
		err_log("failed to allocate thread_id buffer\n");
		return -ENOMEM;
	}

	// prepare arguments for threads
	struct thread_arg_t *targ = (struct thread_arg_t *)malloc(
			nthread * sizeof(struct thread_arg_t));
	if (!targ) {
		err_log("failed to allocate thread argument buffer\n");
		return -ENOMEM;
	}

	struct decoder_context *ctx = NULL;
	for (i = 0; i < nthread; i++) {
		targ[i].in = &in;
		ctx = init_ctx(device, &in);
		if (!ctx)
			return -ENOMEM;
		targ[i].ctx = ctx;
		set_buffers(ctx);
		memcpy(ctx->ec_ctx->data.buf, data, frame_size);
	}

	// populate the code blocks of the first ctx
	struct ec_context *ec_ctx = targ[0].ctx->ec_ctx;
	int code_size = block_size * in.m;
	memset(ec_ctx->code.buf, 0, code_size);
	err = sw_ec_encode(ec_ctx);
	if (err) {
		err_log("Failed sw_ec_encode (%d)\n", err);
		return err;
	}
	
	// populate the code blocks of other ctx
	for (i = 1; i < nthread; i++) {
		ec_ctx = targ[i].ctx->ec_ctx;
		memcpy(ec_ctx->code.buf, targ[0].ctx->ec_ctx->code.buf, code_size);
	}

	// timing
	struct tms	tmsstart, tmsend;
	clock_t		start, end;
	start = Times(&tmsstart);

	for (i = 0; i < nthread; i++)
		Pthread_create(&tid[i], NULL, thread, &targ[i]);

	for (i = 0; i < nthread; i++)
		Pthread_join(tid[i], NULL);

	end = Times(&tmsend);
	double time_real = 0.0;
	// output used time
	pr_times(end - start, &tmsstart, &tmsend, &time_real, NULL, NULL);
	printf("%.2f\n", time_real);

	// clean up
	for (i = 0; i < nthread; i++) {
		ctx = targ[i].ctx;
		free(ctx->ec_ctx->data_arr);
		free(ctx->ec_ctx->code_arr);
		close_ctx(ctx);
	}
	free(tid);
	free(targ);
	free(data);
	return 0;
}

void *thread(void *vargp) 
{
	int err;
	struct thread_arg_t *arg = (struct thread_arg_t *)vargp;
	// encode data
	err = decode_benchmark(arg->ctx, arg->in);
	if (err)
		err_log("failed to encode\n");

	return NULL;
}
