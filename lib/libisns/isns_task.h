/*	$NetBSD: isns_task.h,v 1.1.1.1 2011/01/16 01:22:50 agc Exp $	*/

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
 * isns_task.h
 */

#ifndef _ISNS_TASK_H_
#define _ISNS_TASK_H_

#include <sys/event.h>

#define ISNS_TASK_DISCOVER_SERVER	0
#define ISNS_TASK_RECONNECT_SERVER	1
#define ISNS_TASK_SEND_PDU		2
#define ISNS_TASK_INIT_SOCKET_IO	3
#define ISNS_TASK_INIT_REFRESH		4
#define ISNS_NUM_TASKS			5


union isns_task_var_u {
	struct {
		struct isns_trans_s *trans_p;
		struct isns_pdu_s *pdu_p;
	} send_pdu;

	struct {
		struct addrinfo *ai_p;
	} reconnect_server;

	struct {
		isns_socket_t sd;
		struct addrinfo *ai_p;
	} init_socket_io;

	struct {
		struct isns_refresh_s *ref_p;
	} init_refresh;

	void *data_p;
};

struct isns_task_s {
	uint8_t task_type;
	struct isns_config_s *cfg_p;
	union isns_task_var_u var;

	int waitable;
	pthread_mutex_t wait_mutex;
	pthread_cond_t wait_condvar;
	int wait_ref_count;

	SIMPLEQ_ENTRY(isns_task_s) taskq_entry;
};

typedef void (isns_task_handler)(struct isns_task_s *);

void isns_run_task(struct isns_task_s *);
void isns_end_task(struct isns_task_s *);
int isns_wait_task(struct isns_task_s *, const struct timespec *);

struct isns_task_s *isns_new_task(struct isns_config_s *, uint8_t, int);
void isns_free_task(struct isns_task_s *);
void isns_taskq_insert_tail(struct isns_config_s *, struct isns_task_s *);
void isns_taskq_insert_head(struct isns_config_s *, struct isns_task_s *);
struct isns_task_s *isns_taskq_remove(struct isns_config_s *);
struct isns_task_s *isns_taskq_remove_trans(struct isns_config_s *, uint16_t);


#endif /* !_ISNS_TASK_H_ */
