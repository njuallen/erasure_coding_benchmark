/*
 * Copyright (c) 2015 Mellanox Technologies.  All rights reserved.
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

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/times.h>
#include <netdb.h>
#include <malloc.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <infiniband/verbs.h>

#define dbg_log		if (debug) printf
#define info_log	if (verbose) printf
#define err_log		printf

enum long_opt_value {optval_max_inflight_calcs = 256};

struct inargs {
    char	*devname;
    int	k;
    int	m;
    // Galois field GF(2^w)
    int	w;
    char    *datafile;
    char    *codefile;
    /* size of file in bytes
     * we use long long int
     * since the NIC is so powerful
     * only very large files can make it busy
     */
    long long int file_size;
    // size of EC frame
    int	frame_size;
    // number of threads used
    int nthread;
    // string of failed_blocks
    char	*failed_blocks;
    // ??????????
    int	depth;
    // ??????????
    int	duration;
    int max_inflight_calcs;
    // using sync verbs api?
    int sync_verbs;
    // using async verbs api?
    int async_verbs;
    // using software EC?
    int	jerasure;
};

extern struct sockaddr_storage ssin;
extern uint16_t port;
extern int verbose;
extern int debug;

static inline unsigned long align_any(unsigned long val, unsigned long algn)
{
	return val + ((val % algn) ? (algn - (val % algn)) : 0);
}

int common_process_inargs(int argc, char *argv[], 
			  char *opt_str,
			  struct option *long_options,
			  struct inargs *in,
			  void (*usage)(const char *));

int get_addr(char *dst, struct sockaddr *addr);

#endif /* COMMON_H */
