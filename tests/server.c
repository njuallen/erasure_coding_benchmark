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
    printf("  -p, --ib-port-index=<index>  IB port index\n");
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

    /* 这个ib_port_index是我们给机器上所有的interface编个号的那个号 */
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
        char srv_name[HRD_QP_NAME_SIZE];
        sprintf(srv_name, "server-conn-%d", i);
        hrd_publish_conn_qp(cb, i, srv_name);
    }

    printf("main: Master published all QPs on port %d\n", ib_port_index);

    hrd_red_printf("main: Master published all QPs. Waiting for clients..\n");

    for(i = 0; i < num_clients; i++) {
        char clt_conn_qp_name[HRD_QP_NAME_SIZE];
        sprintf(clt_conn_qp_name, "client-conn-%d", i);
        printf("main: Master waiting for client %s\n", clt_conn_qp_name);

        struct hrd_qp_attr *clt_qp = NULL;
        while(clt_qp == NULL) {
            clt_qp = hrd_get_published_qp(clt_conn_qp_name);
            if(clt_qp == NULL) {
                usleep(200000);
            }
        }

        printf("main: Master found client %d of %d clients. Connecting..\n",
                i, num_clients);

        hrd_connect_qp(cb, i, clt_qp);

        char mstr_qp_name[HRD_QP_NAME_SIZE];
        sprintf(mstr_qp_name, "server-%d", i);
        hrd_publish_ready(mstr_qp_name);
    }


    struct ibv_recv_wr *recv_wrs = Calloc(HRD_Q_DEPTH * num_clients, 
            sizeof(struct ibv_recv_wr));

    struct ibv_sge *sges = Calloc(HRD_Q_DEPTH * num_clients,
            sizeof(struct ibv_sge));

    for(i = 0; i < HRD_Q_DEPTH * num_clients; i++) {
        sges[i].length = block_size;
        sges[i].lkey = cb->conn_buf_mr->lkey;
        sges[i].addr = (uint64_t)((char *)cb->conn_buf + i * block_size);
        // with this field, we can distinguish CQEs
        recv_wrs[i].wr_id = i;
        recv_wrs[i].sg_list = &sges[i];
        recv_wrs[i].num_sge = 1;
        recv_wrs[i].next = NULL;
    }

    int ret;

    struct ibv_recv_wr *bad_wr;
    // server should post recvs first
    for(i = 0; i < num_clients; i++) {
        for(j = 0; j < HRD_Q_DEPTH; j++) {
            ret = ibv_post_recv(cb->conn_qp[i], &recv_wrs[i * HRD_Q_DEPTH + j], &bad_wr);
            if(ret)
                unix_error("ibv_post_recv");
        }
    }

    for(i = 0; i < num_clients; i++) {
        char mstr_qp_name[HRD_QP_NAME_SIZE];
        sprintf(mstr_qp_name, "server-%d", i);
        hrd_publish_ready(mstr_qp_name);
    }

    struct ibv_wc wc[HRD_Q_DEPTH];

    // clean up
    while(1) {
        for(i = 0; i < num_clients; i++) {
            int ret = ibv_poll_cq(cb->conn_cq[i], HRD_Q_DEPTH, wc);
            CPE(ret < 0, "ibv_poll_cq failed", -1);
            for(j = 0; j < ret; j++) {
                CPE(wc[j].status != 0, "Bad wc status", -1);
                // post the recv to the receive queue again
                int id = wc[j].wr_id;
                recv_wrs[id].wr_id = id;
                recv_wrs[id].sg_list = &sges[id];
                recv_wrs[id].num_sge = 1;
                recv_wrs[id].next = NULL;
                // printf("client: %s", (char *)sges[id].addr);
                ret = ibv_post_recv(cb->conn_qp[i], &recv_wrs[id], &bad_wr);
                CPE(ret, "ibv_post_recv", -1);
            }
        }
    }
    return 0;
}
