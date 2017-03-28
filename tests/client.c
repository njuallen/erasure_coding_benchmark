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
        { .name = "frame_size",    .has_arg = 1, .val = 's' },
        { .name = "data_blocks",   .has_arg = 1, .val = 'k' },
        { .name = "code_blocks",   .has_arg = 1, .val = 'm' },
        { .name = "debug",         .has_arg = 0, .val = 'd' },
        { .name = "verbose",       .has_arg = 0, .val = 'v' },
        { .name = "help",          .has_arg = 0, .val = 'h' },
        { .name = 0, .has_arg = 0, .val = 0 }
    };

    err = common_process_inargs(argc, argv, "i:p:g:s:k:m:hdv",
            long_options, in, usage);
    if (err)
        return err;

    return 0;
}

int main(int argc, char *argv[]) {
    struct inargs in;
    int k, m;
    int err;
    int i, j;

    err = process_inargs(argc, argv, &in);
    if (err)
        return err;

    k = in.k;
    m = in.m;

    int num_clients = k + m;

    int block_size = align_any((in.frame_size + k - 1) / k, 64);
    /* since the maximum queue depth is HRD_Q_DEPTH
     * we need to be able to populate all the entries in the receive queue
     */
    int recv_buf_size = block_size * HRD_Q_DEPTH * num_clients;

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
    struct hrd_ctrl_blk *cb = hrd_ctrl_blk_init(1,	/* local hid */
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

    struct ibv_send_wr *send_wrs = Calloc(HRD_Q_DEPTH * num_clients, 
            sizeof(struct ibv_send_wr));

    struct ibv_sge *sges = Calloc(HRD_Q_DEPTH * num_clients,
            sizeof(struct ibv_sge));

    for(i = 0; i < HRD_Q_DEPTH * num_clients; i++) {
        sges[i].length = block_size;
        sges[i].lkey = cb->conn_buf_mr->lkey;
        sges[i].addr = (uint64_t)((char *)cb->conn_buf + i * block_size);
        strcpy((char *)sges[i].addr, "miao miao miao!\n");
        // with this field, we can distinguish CQEs
        send_wrs[i].wr_id = i;
        send_wrs[i].sg_list = &sges[i];
        send_wrs[i].num_sge = 1;
        send_wrs[i].next = NULL;
        send_wrs[i].opcode = IBV_WR_SEND;
        send_wrs[i].send_flags = IBV_SEND_SIGNALED;
    }

    int ret;

    struct ibv_send_wr *bad_wr;
    // server should post sends first
    for(i = 0; i < num_clients; i++) {
        for(j = 0; j < HRD_Q_DEPTH; j++) {
            ret = ibv_post_send(cb->conn_qp[i], &send_wrs[i * HRD_Q_DEPTH + j], &bad_wr);
            CPE(ret, "ibv_post_send", -1);
        }
    }

    struct ibv_wc wc[HRD_Q_DEPTH];

    // clean up
    int cnt = 0;
    while(1) {
        for(i = 0; i < num_clients; i++) {
            int ret = ibv_poll_cq(cb->conn_cq[i], HRD_Q_DEPTH, wc);
            CPE(ret < 0, "ibv_poll_cq failed", -1);
            for(j = 0; j < ret; j++) {
                if(wc[j].status != 0) 
                    printf("Bad wc status: %s\n", ibv_wc_status_str(wc[j].status));
                CPE(wc[j].status != 0, "Bad wc status", -1);
                printf("cnt: %d\n", ++cnt);
                // post the send to the receive queue again
                int id = wc[j].wr_id;
                send_wrs[id].wr_id = id;
                send_wrs[id].sg_list = &sges[id];
                send_wrs[id].num_sge = 1;
                send_wrs[id].next = NULL;
                send_wrs[i].opcode = IBV_WR_SEND;
                send_wrs[i].send_flags = IBV_SEND_SIGNALED;
                ret = ibv_post_send(cb->conn_qp[i], &send_wrs[id], &bad_wr);
                CPE(ret, "ibv_post_send", -1);
            }
        }
    }
    return 0;
}
