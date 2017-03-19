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

	err = common_process_inargs(argc, argv, "i:F:s:k:m:w:t:VShdv",
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
long long *iterations;
uint8_t **data_arr;
uint8_t **code_arr;
uint8_t *data, *code;
int block_size;
int frame_size;
int code_size;
int *matrix;
int k, m, w;
int shmid;
int shmkey = 12222;

static int encode_benchmark(void) {
    int i;
    // the number jobs that this process do
    long long jobs = 0;

    while(1) {
        long long batch = 20;
        // use batching to avoid much contention
        long long iter = __sync_fetch_and_sub(iterations, batch);
        // finished
        if(iter <= 0)
            break;

        // the number of operations to do this time
        int nops = (iter < batch) ? iter : batch;
        jobs += nops;

        for(i = 0; i < nops; i++) {
            memset(code, 0, block_size * m);
            jerasure_matrix_encode(k, m, w, matrix, (char **)data_arr, (char **)code_arr, block_size);
        }
    }
    // use this to check whether the total number of jobs done by all processes are correct
    dbg_log("jobs: %lld\n", jobs);
    return 0;
}

static void set_buffers()
{
    int i;
    data_arr = Calloc(k, sizeof(*data_arr));
    code_arr = Calloc(m, sizeof(*code_arr));

    for (i = 0; i < k ; i++) {
        data_arr[i] = data + i * block_size;
    }
    for (i = 0; i < m ; i++) {
        code_arr[i] = code + i * block_size;
    }
}

int main(int argc, char *argv[]) {
    struct inargs in;
    int err;

    err = process_inargs(argc, argv, &in);
    if (err)
        return err;

    k = in.k;
    m = in.m;
    w = in.w;

    shmid = Shmget(shmkey, sizeof(long long), IPC_CREAT | IPC_EXCL | S_IRWXU);
    iterations = Shmat(shmid, NULL, 0);

    block_size = align_any((in.frame_size + k - 1) / k, 64);
    // the real frame_size we use after aligning block_size to 64 bytes
    frame_size = block_size * k;
    code_size = block_size * m;
    *iterations = in.file_size / frame_size;

    dbg_log("iterations: %lld\nblock_size: %d\nframe_size: %d\n", 
            *iterations, block_size, frame_size);

    /* eventually, every child process will 
     * get their own copy of data code due to COW
     */
    data = (uint8_t *)Malloc(sizeof(uint8_t) * frame_size);
    code = (uint8_t *)Malloc(sizeof(uint8_t) * code_size);

    set_buffers();

    int i = 0;
    srand(time(NULL));
    for(i = 0; i < frame_size; i++)
        data[i] = rand();

    // allocate encode matrix
    matrix = reed_sol_vandermonde_coding_matrix(k, m, w);
    if (!matrix) {
        err_log("failed to allocate reed sol matrix\n");
        return -EINVAL;
    }

    // timing
    struct tms	tmsstart, tmsend;
    clock_t		start, end;
    start = Times(&tmsstart);

    int nprocess = in.nthread;
    pid_t *pid = (pid_t *)Malloc(nprocess * sizeof(pid_t)); 
    for(i = 0; i < nprocess; i++) {
        pid[i] = Fork();
        if(pid[i] == 0) {
            // child proccess do encoding
            iterations = Shmat(shmid, NULL, 0);
            encode_benchmark();
            Shmdt(iterations);
            exit(0);
        }
    }

    // wait for all child to exit
    for(i = 0; i < nprocess; i++)
        Waitpid(pid[i], NULL, 0);

    end = Times(&tmsend);
    double time_real = 0.0;
    // output used time
    pr_times(end - start, &tmsstart, &tmsend, &time_real, NULL, NULL);
    printf("%.2f\n", time_real);

    // clean up
    free(pid);
    free(data);
    free(code);
    free(matrix);
    Shmdt(iterations);
    Shmctl(shmid, IPC_RMID, NULL);
    return 0;
}
