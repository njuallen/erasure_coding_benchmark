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
#include "hrd.h"

#define Q_WR_BUFFER_SIZE HRD_Q_DEPTH

static void usage(const char *argv0)
{
	printf("Usage:\n");
	printf("  %s            start EC encoder\n", argv0);
	printf("\n");
	printf("Options:\n");
	printf("  -i, --ib-dev=<dev>           use IB device <dev> (default first device found)\n");
    printf("  -p, --ib-port-index=<dev>    IB port index\n");
    printf("  -g, --gid-index=<index>      GID index\n");
	printf("  -k, --data_blocks=<blocks>   Number of data blocks\n");
	printf("  -m, --code_blocks=<blocks>   Number of code blocks\n");
	printf("  -w, --gf=<gf>                Galois field GF(2^w)\n");
	printf("  -F, --file_size=<size>       size of file in bytes\n");
	printf("  -s, --frame_size=<size>      size of EC frame\n");
	printf("  -d, --debug                  print debug messages\n");
	printf("  -v, --verbose                add verbosity\n");
	printf("  -h, --help                   display this output\n");
}

static int process_inargs(int argc, char *argv[], struct inargs *in)
{
    int err;
    struct option long_options[] = {
        { .name = "ib-dev",        .has_arg = 1, .val = 'i' },
        { .name = "ib-port-index", .has_arg = 1, .val = 'p' },
        { .name = "gid-index",     .has_arg = 1, .val = 'g' },
        { .name = "file_size",     .has_arg = 1, .val = 'F' },
        { .name = "frame_size",    .has_arg = 1, .val = 's' },
        { .name = "data_blocks",   .has_arg = 1, .val = 'k' },
        { .name = "code_blocks",   .has_arg = 1, .val = 'm' },
        { .name = "gf",            .has_arg = 1, .val = 'w' },
        { .name = "debug",         .has_arg = 0, .val = 'd' },
        { .name = "verbose",       .has_arg = 0, .val = 'v' },
        { .name = "help",          .has_arg = 0, .val = 'h' },
        { .name = 0, .has_arg = 0, .val = 0 }
    };

    err = common_process_inargs(argc, argv, "i:p:g:F:s:k:m:w:hdv",
            long_options, in, usage);
    if (err)
        return err;

    return 0;
}

long long iterations;
uint8_t **data_arr;
uint8_t **code_arr;
uint8_t *data, *code;
int block_size;
int frame_size;
int code_size;
int *matrix;
int k, m, w;
int num_clients;
struct hrd_ctrl_blk *cb;
struct ibv_send_wr *send_wrs;
struct ibv_sge *sges;
struct queue buffer_queue;
int *outstanding_send;

int is_ready_to_send() {
    // no available memory blocks
    if(buffer_queue.cnt < num_clients)
        return 0;
    int i;
    // the send queue is full
    for(i = 0; i < num_clients; i++) {
        if(outstanding_send[i] == HRD_Q_DEPTH)
            return 0;
    }
    return 1;
}

static int encode_benchmark(void) {
    int i, j;
    int ret;
    data_arr = Malloc(sizeof(uint8_t *) * k);
    code_arr = Malloc(sizeof(uint8_t *) * m);

    struct ibv_send_wr **wrs = Calloc(num_clients, sizeof(struct ibv_send_wr *));
    outstanding_send = Calloc(num_clients, sizeof(int));

    struct ibv_send_wr *bad_wr;
    while(iterations) {
        if(is_ready_to_send()) {
            struct ibv_send_wr *wr;
            for(i = 0; i < k; i++) {
                wr = dequeue(&buffer_queue);
                data_arr[i] = (uint8_t *)wr->sg_list[0].addr;
                memcpy(data_arr[i], data + block_size * i, block_size);
                wrs[i] = wr;
            }
            for(i = 0; i < m; i++) {
                wr = dequeue(&buffer_queue);
                code_arr[i] = (uint8_t *)wr->sg_list[0].addr;
                memset(code_arr[i], 0, block_size);
                wrs[k + i] = wr;
            }
            jerasure_matrix_encode(k, m, w, matrix, (char **)data_arr, (char **)code_arr, block_size);
            // now we need to send data and code blocks
            // server should post sends first
            for(i = 0; i < num_clients; i++) {
                ret = ibv_post_send(cb->conn_qp[i], wrs[i], &bad_wr);
                outstanding_send[i]++;
                CPE(ret, "ibv_post_send", -1);
            }
            iterations--;
        }
        struct ibv_wc wc[HRD_Q_DEPTH];
        for(i = 0; i < num_clients; i++) {
            ret = ibv_poll_cq(cb->conn_cq[i], HRD_Q_DEPTH, wc);
            CPE(ret < 0, "ibv_poll_cq failed", -1);
            outstanding_send[i] -= ret;
            for(j = 0; j < ret; j++) {
                int id = wc[j].wr_id;
                send_wrs[id].wr_id = id;
                send_wrs[id].sg_list = &sges[id];
                send_wrs[id].num_sge = 1;
                send_wrs[id].next = NULL;
                send_wrs[i].opcode = IBV_WR_SEND;
                send_wrs[i].send_flags = IBV_SEND_SIGNALED;
                if(wc[j].status != 0) {
                    // failed
                    printf("Bad wc status: %s\n", ibv_wc_status_str(wc[j].status));
                    exit(0);
                }
                else
                    enqueue(&buffer_queue, &send_wrs[i]);
            }
        }
    }
    return 0;
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

    block_size = align_any((in.frame_size + k - 1) / k, 64);
    // the real frame_size we use after aligning block_size to 64 bytes
    frame_size = block_size * k;
    code_size = block_size * m;
    iterations = in.file_size / frame_size;

    dbg_log("iterations: %lld\nblock_size: %d\nframe_size: %d\n", 
            iterations, block_size, frame_size);

    data = (uint8_t *)Malloc(sizeof(uint8_t) * frame_size);

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

    num_clients = k + m;

    block_size = align_any((in.frame_size + k - 1) / k, 64);

    int recv_buf_size = block_size * Q_WR_BUFFER_SIZE * num_clients;

    /* 这个ib_port_index是我们给机器上所有的interface编个号的那个号
     * 即同一个机器上，所有网卡的所有port放在一起编的那个号
     * libhrd用到了这个
     */
    int ib_port_index = in.ib_port_index;

    int gid_index = in.gid_index;

    /* from herds api:
     * local_hid can be anything, 
     * it's just control block identifier that is
     * useful in printing debug info.
     *
     * do not use huge page
     * use RC
     */
    cb = hrd_ctrl_blk_init(1,	/* local hid */
            ib_port_index, gid_index, -1, /* port index, numa node id */
            num_clients, 0,	/* #conn_qps, use_uc */
            NULL, recv_buf_size, -1,
            0, 0, -1);	/* #dgram qps, buf size, shm key */

    /* Zero out the request buffer */
    memset((void *) cb->conn_buf, 0, recv_buf_size);

    /* Register all created QPs ! */
    for(i = 0; i < num_clients; i++) {
        char client_name[HRD_QP_NAME_SIZE];
        sprintf(client_name, "client-conn-%d", i);
        hrd_publish_conn_qp(cb, i, client_name);
    }

    printf("main: Client published all QPs on port %d\n", ib_port_index);

    for(i = 0; i < num_clients; i++) {
        char srv_conn_qp_name[HRD_QP_NAME_SIZE];
        sprintf(srv_conn_qp_name, "server-conn-%d", i);
        printf("main: client search for server_qp: %s\n", srv_conn_qp_name);

        struct hrd_qp_attr *srv_qp = NULL;
        while(srv_qp == NULL) {
            srv_qp = hrd_get_published_qp(srv_conn_qp_name);
            if(srv_qp == NULL) {
                usleep(200000);
            }
        }

        printf("main: Client found server qp %d of %d. Connecting..\n",
                i, num_clients);

        hrd_connect_qp(cb, i, srv_qp);
        char mstr_qp_name[HRD_QP_NAME_SIZE];
        sprintf(mstr_qp_name, "server-%d", i);
        hrd_wait_till_ready(mstr_qp_name);
    }

    send_wrs = Calloc(Q_WR_BUFFER_SIZE * num_clients, 
            sizeof(struct ibv_send_wr));

    sges = Calloc(Q_WR_BUFFER_SIZE * num_clients,
            sizeof(struct ibv_sge));

    queue_init(&buffer_queue, Q_WR_BUFFER_SIZE * num_clients);

    for(i = 0; i < Q_WR_BUFFER_SIZE * num_clients; i++)
        enqueue(&buffer_queue, &send_wrs[i]);

    for(i = 0; i < Q_WR_BUFFER_SIZE * num_clients; i++) {
        sges[i].length = block_size;
        sges[i].lkey = cb->conn_buf_mr->lkey;
        sges[i].addr = (uint64_t)((char *)cb->conn_buf + i * block_size);
        // with this field, we can distinguish CQEs
        send_wrs[i].wr_id = i;
        send_wrs[i].sg_list = &sges[i];
        send_wrs[i].num_sge = 1;
        send_wrs[i].next = NULL;
        send_wrs[i].opcode = IBV_WR_SEND;
        send_wrs[i].send_flags = IBV_SEND_SIGNALED;
    }
    // timing
    struct tms	tmsstart, tmsend;
    clock_t		start, end;
    start = Times(&tmsstart);

    encode_benchmark();

    end = Times(&tmsend);
    double time_real = 0.0;
    // output used time
    pr_times(end - start, &tmsstart, &tmsend, &time_real, NULL, NULL);
    printf("%.2f\n", time_real);

    // clean up
    free(data);
    free(code);
    free(matrix);
    return 0;
}
