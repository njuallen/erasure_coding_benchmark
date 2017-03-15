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

#include "common.h"

struct sockaddr_storage ssin;
uint16_t port;
int verbose = 0;
int debug = 0;


int common_process_inargs(int argc, char *argv[], 
			  char *opt_str,
			  struct option *long_options,
			  struct inargs *in,
			  void (*usage)(const char *))
{
	in->devname = NULL;
	in->file_size = 0LL;
	in->frame_size = 1000;
	in->datafile = NULL;
	in->codefile = NULL;
	in->k = 10;
	in->m = 4;
	in->w = 4;
	in->nthread = 1;
	in->verbs = 0;
	in->sw = 0;
	in->mlx_lib = 0;

	while (1) {
		int c, ret;

		c = getopt_long(argc, argv, opt_str, long_options, NULL);
		if (c == -1)
			break;

		switch (c) {
		case 'a':
			ret = get_addr(optarg, (struct sockaddr *)&ssin);
			break;

		case 'i':
			ret = asprintf(&in->devname, optarg);
			if (ret < 0) {
				err_log("failed devname asprintf\n");
				return ret;
			}
			break;

		case 'D':
			ret = asprintf(&in->datafile, optarg);
			if (ret < 0) {
				err_log("failed data file asprintf\n");
				return ret;
			}
			break;

		case 'C':
			ret = asprintf(&in->codefile, optarg);
			if (ret < 0) {
				err_log("failed code file asprintf\n");
				return ret;
			}
			break;

		case 'E':
			ret = asprintf(&in->failed_blocks, optarg);
			if (ret < 0) {
				usage(argv[0]);
				return -EINVAL;
			}
			break;

		case 'F':
			in->file_size = strtoll(optarg, NULL, 0);
			if (in->file_size < 0) {
				usage(argv[0]);
				return -EINVAL;
			}
			break;

		case 't':
			in->nthread = strtol(optarg, NULL, 0);
			if (in->nthread < 0) {
				usage(argv[0]);
				return -EINVAL;
			}
			break;

		case 's':
			in->frame_size = strtol(optarg, NULL, 0);
			if (in->frame_size < 0) {
				usage(argv[0]);
				return -EINVAL;
			}
			break;

		case 'k':
			in->k = strtol(optarg, NULL, 0);
			if (in->k <= 0 || in->k > 16) {
				usage(argv[0]);
				return -EINVAL;
			}
			break;

		case 'm':
			in->m = strtol(optarg, NULL, 0);
			if (in->m <= 0 || in->m > 4) {
				usage(argv[0]);
				return -EINVAL;
			}
			break;

		case 'w':
			in->w = strtol(optarg, NULL, 0);
			if (in->w != 1 && in->w != 2 && in->w != 4) {
				usage(argv[0]);
				return -EINVAL;
			}
			break;

		case 'q':
			in->depth = strtol(optarg, NULL, 0);
			if (in->depth <= 0) {
				usage(argv[0]);
				return -EINVAL;
			}
			break;

		case 'r':
			in->duration = strtol(optarg, NULL, 0);
			if (in->duration <= 0) {
				usage(argv[0]);
				return -EINVAL;
			}
			break;

		case 'S':
			in->sw = 1;
			break;

		case 'V':
			in->verbs = 1;
			break;

		case 'L':
			in->mlx_lib = 1;
			break;

		case 'v':
			verbose = 1;
			break;

		case 'd':
			debug = 1;
			break;

		case 'h':
			usage(argv[0]);
			exit(0);

		default:
			usage(argv[0]);
			return -EINVAL;
		}
	}

	return 0;
}

int get_addr(char *dst, struct sockaddr *addr)
{
	struct addrinfo *res;
	int ret;

	ret = getaddrinfo(dst, NULL, NULL, &res);
	if (ret) {
		printf("getaddrinfo failed - invalid hostname or IP address\n");
		return ret;
	}

	if (res->ai_family == PF_INET)
		memcpy(addr, res->ai_addr, sizeof(struct sockaddr_in));
	else if (res->ai_family == PF_INET6)
		memcpy(addr, res->ai_addr, sizeof(struct sockaddr_in6));
	else
		ret = -1;
	
	freeaddrinfo(res);

	return ret;
}

// copied form apue 3e
// print the real, sys and user time between tmsstart and tmsend
void pr_times(clock_t real_clock, struct tms *tmsstart, struct tms *tmsend, 
		double *real, double *sys, double *user)
{
	static long	clktck = 0;
	/* fetch clock ticks per second first time */
	if (clktck == 0)
		if ((clktck = sysconf(_SC_CLK_TCK)) < 0)
			app_error("sysconf error");

	if(real)
		*real = real_clock / (double) clktck;
	if(sys)
		*sys = (tmsend->tms_stime - tmsstart->tms_stime) / (double) clktck;
	if(user)
		*user =(tmsend->tms_utime - tmsstart->tms_utime) / (double) clktck;
}

// posix-style error
void posix_error(int code, char *msg) 
{
	fprintf(stderr, "%s: %s\n", msg, strerror(code));
	exit(0);
}

// application error
void app_error(char *msg)
{
	fprintf(stderr, "%s\n", msg);
	exit(0);
}

// error handling wrapper for posix times function
clock_t Times(struct tms *buffer) {
	clock_t ret = times(buffer);
	if(ret == -1)
		posix_error(errno, "times error");
	return ret;
}

/************************************************
 * Wrappers for Pthreads thread control functions
 * Copied from csapp
 ************************************************/

void Pthread_create(pthread_t *tidp, pthread_attr_t *attrp, 
		    void * (*routine)(void *), void *argp) 
{
    int rc;

    if ((rc = pthread_create(tidp, attrp, routine, argp)) != 0)
	posix_error(rc, "Pthread_create error");
}

void Pthread_cancel(pthread_t tid) {
    int rc;

    if ((rc = pthread_cancel(tid)) != 0)
	posix_error(rc, "Pthread_cancel error");
}

void Pthread_join(pthread_t tid, void **thread_return) {
    int rc;

    if ((rc = pthread_join(tid, thread_return)) != 0)
	posix_error(rc, "Pthread_join error");
}

/* $begin detach */
void Pthread_detach(pthread_t tid) {
    int rc;

    if ((rc = pthread_detach(tid)) != 0)
	posix_error(rc, "Pthread_detach error");
}
/* $end detach */

void Pthread_exit(void *retval) {
    pthread_exit(retval);
}

pthread_t Pthread_self(void) {
    return pthread_self();
}
 
void Pthread_once(pthread_once_t *once_control, void (*init_function)()) {
    pthread_once(once_control, init_function);
}

