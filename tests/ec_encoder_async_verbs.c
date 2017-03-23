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
#include "utils.h"

struct encoder_context {
	struct ibv_context	*context;
	struct ibv_pd		*pd;
	struct ec_context	*ec_ctx;
};

struct thread_arg_t {
    struct producer_consumer pc;
    struct inargs *in;
    struct encoder_context *ctx;
};

// set maximum inflight calculations to 10
int max_inflight;

static struct encoder_context *
init_ctx(struct ibv_device *ib_dev, struct inargs *in)
{
    struct encoder_context *ctx;

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
            in->k, in->m, in->w, max_inflight, NULL);
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

static void close_ctx(struct encoder_context *ctx)
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
	printf("  -F, --file_size=<size>       size of file in bytes\n");
	printf("  -s, --frame_size=<size>      size of EC frame\n");
	printf("  -t, --thread_number=<number> number of threads used\n");
	printf("  -d, --debug                  print debug messages\n");
	printf("  -v, --verbose                add verbosity\n");
	printf("  -h, --help                   display this output\n");
}

static int process_inargs(int argc, char *argv[], struct inargs *in)
{
    int err;
    struct option long_options[] = {
        { .name = "ib-dev",        .has_arg = 1, .val = 'i' },
        { .name = "file_size",     .has_arg = 1, .val = 'F' },
        { .name = "frame_size",    .has_arg = 1, .val = 's' },
        { .name = "data_blocks",   .has_arg = 1, .val = 'k' },
        { .name = "code_blocks",   .has_arg = 1, .val = 'm' },
        { .name = "gf",            .has_arg = 1, .val = 'w' },
        { .name = "thread_number", .has_arg = 1, .val = 't' },
        { .name = "max_inflight",  .has_arg = 1, .val = optval_max_inflight_calcs },
        { .name = "debug",         .has_arg = 0, .val = 'd' },
        { .name = "verbose",       .has_arg = 0, .val = 'v' },
        { .name = "help",          .has_arg = 0, .val = 'h' },
        { .name = 0, .has_arg = 0, .val = 0 }
    };

    err = common_process_inargs(argc, argv, "i:F:s:k:m:w:t:hdv",
            long_options, in, usage);
    if (err)
        return err;

    return 0;
}

/* the total number of encode operations needs to be done
 * all threads share this counter
 * each time, each thread gets some work and decreases this counter
 * If the value of the counter is below zero,
 * there is no more work, threads can exit.
 */
volatile long long iterations = 0;

struct async_ec_comp {
    struct ibv_exp_ec_comp comp;
    struct ec_mem *mem;
    struct producer_consumer *pc;
};

static void async_encode_done(struct ibv_exp_ec_comp *comp) {
    if(comp->status != IBV_EXP_EC_CALC_SUCCESS)
        app_error("async_ec calculation failed!");

    struct async_ec_comp *ec_comp = (void *)comp - offsetof(struct async_ec_comp, comp);
    produce(ec_comp->pc, ec_comp->mem);
    Free(ec_comp);
}

static int encode_benchmark(struct encoder_context *ctx, struct producer_consumer *pc) {
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

        // uses async verbs API
        for(i = 0; i < nops; i++) {
            // get a memory block to work on
            struct ec_mem *mem = consume(pc);
            memset(mem->code.buf, 0, ec_ctx->block_size * ec_ctx->attr.m);
            struct async_ec_comp *comp = Calloc(1, sizeof(struct async_ec_comp));
            comp->comp.done = async_encode_done;
            comp->mem = mem;
            comp->pc = pc;
            err = ibv_exp_ec_encode_async(ec_ctx->calc, &mem->mem, &comp->comp);
            if (err) {
                err_log("Failed ibv_exp_ec_encode (%d)\n", err);
                return err;
            }
        }
    }
    /* wait for in flight calculations to finish
     * use this to make sure that all producers has finished their job
     */
    for(i = 0; i < max_inflight; i++)
        consume(pc);
    return 0;
}

void *thread(void *vargp);

int main(int argc, char *argv[]) {
    struct ibv_device *device;
    struct inargs in;
    int err;

    err = process_inargs(argc, argv, &in);
    if (err)
        return err;

    max_inflight = in.max_inflight_calcs;

    info_log("max_inflight_calcs: %d\n", max_inflight);

    device = find_device(in.devname);
    if (!device)
        return -EINVAL;

    int block_size = align_any((in.frame_size + in.k - 1) / in.k, 64);
    // the real frame_size we use after aligning block_size to 64 bytes
    int frame_size = block_size * in.k;
    iterations = in.file_size / frame_size;

    dbg_log("iterations: %lld\nblock_size: %d\nframe_size: %d\n", 
            iterations, block_size, frame_size);

    /* generate the data blocks to encode
     * we should generate data blocks in the main thread
     * so that the long running time of rand will not be accounted
     */
    uint8_t *data = (uint8_t *)Malloc(sizeof(uint8_t) * frame_size);
    int i = 0;
    srand(time(NULL));
    for(i = 0; i < frame_size; i++)
        data[i] = rand();

    int nthread = in.nthread;
    pthread_t *tid = (pthread_t *)Malloc(nthread * sizeof(pthread_t)); 

    // prepare arguments for threads
    struct thread_arg_t *targ = (struct thread_arg_t *)Malloc(
            nthread * sizeof(struct thread_arg_t));

    struct encoder_context *ctx = NULL;
    for (i = 0; i < nthread; i++) {
        targ[i].in = &in;
        ctx = init_ctx(device, &in);
        if (!ctx)
            return -ENOMEM;
        targ[i].ctx = ctx;
        producer_consumer_init(&targ[i].pc, max_inflight);

        int j;
        for(j = 0; j < max_inflight; j++) {
            // prepare data buffer
            memcpy(ctx->ec_ctx->buf[j].data.buf, data, frame_size);
            produce(&targ[i].pc, &ctx->ec_ctx->buf[j]);
        }
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
        close_ctx(ctx);
    }
    free(tid);
    free(targ);
    free(data);
    return 0;
}

void *thread(void *vargp) {
    int err;
    struct thread_arg_t *arg = (struct thread_arg_t *)vargp;
    // encode data
    err = encode_benchmark(arg->ctx, &arg->pc);
    if (err)
        err_log("failed to encode\n");

    return NULL;
}
