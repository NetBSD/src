/*	$NetBSD: isns_config.h,v 1.2 2022/04/19 20:32:16 rillig Exp $	*/

/*-
 * Copyright (c) 2004,2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * isns_config.h
 */

#ifndef _ISNS_CONFIG_H_
#define _ISNS_CONFIG_H_

#ifdef ISNS_DEBUG
#define DBG(...)	do { printf(__VA_ARGS__); } while (0)
#else
#define DBG(...)	do {} while (0)
#endif

#include "isns_fileio.h"
#include "isns_pdu.h"
#include "isns_socketio.h"
#include "isns_thread.h"
#include "isns_task.h"
#include "isns_util.h"

enum {
	_ISNS_MUTEXATTR_TYPEMASK	= 0x0000000f,
	ISNS_MUTEX_TYPE_NORMAL 		= 0,
	ISNS_MUTEX_TYPE_ERRORCHECK	= 1,
	ISNS_MUTEX_TYPE_RECURSIVE	= 2,
	ISNS_MUTEX_TYPE_ERRORABORT	= 3
};

struct isns_config_s {
	int kq;
	int pipe_fds[2];

	pthread_t *control_thread_p;

	isns_socket_t sd;
	int sd_connected;
	struct addrinfo *ai_p;
	struct isns_pdu_s *pdu_in_p;

	pthread_mutex_t taskq_mutex;
	struct isns_task_s *curtask_p;
	SIMPLEQ_HEAD(isns_taskq_head_s, isns_task_s) taskq_head;

	pthread_mutex_t trans_mutex;

	int is_server;

	struct isns_refresh_s *refresh_p;
};

#define isns_is_socket_init_done(_cfg_p)				\
	((_cfg_p) == NULL ? 0						\
	    : (((struct isns_config_s *)(_cfg_p))->ai_p != NULL))


#endif /* !_ISNS_CONFIG_H_ */
