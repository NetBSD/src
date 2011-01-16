/*	$NetBSD: isns_util.c,v 1.1.1.1 2011/01/16 01:22:50 agc Exp $	*/

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
 * isns_util.c
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: isns_util.c,v 1.1.1.1 2011/01/16 01:22:50 agc Exp $");


#include <sys/types.h>

#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "isns.h"
#include "isns_config.h"

#define ISNS_MAX_DISCONNECTS_PER_TRANS	3

int
isns_issue_cmd(struct isns_config_s *cfg_p, uint8_t cmd_type)
{
	return (int)write(cfg_p->pipe_fds[1], &cmd_type, 1);
}


int
isns_issue_cmd_with_data(struct isns_config_s *cfg_p, uint8_t cmd_type,
    uint8_t *data_p, int data_len)
{
	struct iovec iov[2];

	iov[0].iov_base = &cmd_type;
	iov[0].iov_len = 1;
	iov[1].iov_base = data_p;
	iov[1].iov_len = data_len;

	return (int)isns_file_writev(cfg_p->pipe_fds[1], iov, 2);
}


int
isns_change_kevent_list(struct isns_config_s *cfg_p,
    uintptr_t ident, uint32_t filter, uint32_t flags, int64_t data, intptr_t udata)
{
	struct kevent evt;

	EV_SET(&evt, ident, filter, flags, 0, data, udata);
	return kevent(cfg_p->kq, &evt, 1, NULL, 0, NULL);
}


struct isns_config_s *
isns_new_config()
{
	struct isns_config_s *cfg_p;
	pthread_mutexattr_t mutexattr;

	cfg_p = (struct isns_config_s *)
	    isns_malloc(sizeof(struct isns_config_s));
	if (cfg_p == NULL) {
		DBG("isns_new_config: error on isns_malloc() [1]\n");
		return NULL;
	}
	cfg_p->kq = -1;
	cfg_p->pipe_fds[0] = -1;
	cfg_p->pipe_fds[1] = -1;
	cfg_p->curtask_p = NULL;
	cfg_p->sd_connected = 0;
	cfg_p->ai_p = NULL;
	cfg_p->pdu_in_p = NULL;

	cfg_p->refresh_p = NULL;

	pthread_mutexattr_init(&mutexattr);
	pthread_mutexattr_settype(&mutexattr, ISNS_MUTEX_TYPE_NORMAL);
	if (pthread_mutex_init(&cfg_p->taskq_mutex, &mutexattr) != 0) {
		DBG("isns_new_config: error on pthread_mutex_init() [1]\n");
		isns_free(cfg_p);
		return NULL;
	}

	pthread_mutexattr_init(&mutexattr);
	pthread_mutexattr_settype(&mutexattr, ISNS_MUTEX_TYPE_NORMAL);
	if (pthread_mutex_init(&cfg_p->trans_mutex, &mutexattr) != 0) {
		DBG("isns_new_config: error on pthread_mutex_init() [2]\n");
		pthread_mutex_destroy(&cfg_p->taskq_mutex);
		isns_free(cfg_p);
		return NULL;
	}

	SIMPLEQ_INIT(&cfg_p->taskq_head);

	cfg_p->control_thread_p = (pthread_t *)isns_malloc(sizeof(pthread_t));
	if (cfg_p->control_thread_p == NULL) {
		DBG("isns_new_config: error on isns_malloc() [1]\n");
		isns_destroy_config(cfg_p);
		return NULL;
	}

	return cfg_p;
}


void
isns_destroy_config(struct isns_config_s *cfg_p)
{
	struct isns_task_s *task_p;

	if (cfg_p != NULL) {
		if (cfg_p->kq != -1)
			close(cfg_p->kq);
		if (cfg_p->pipe_fds[0] != -1)
			close(cfg_p->pipe_fds[0]);
		if (cfg_p->pipe_fds[1] != -1)
			close(cfg_p->pipe_fds[1]);
		if (cfg_p->control_thread_p != NULL)
			isns_free(cfg_p->control_thread_p);
		if (cfg_p->refresh_p != NULL) {
			if (cfg_p->refresh_p->trans_p != NULL)
				isns_free_trans(cfg_p->refresh_p->trans_p);
			isns_free(cfg_p->refresh_p);
		}
		/* Free the current task, if necessary. */
		if ((task_p = cfg_p->curtask_p) != NULL) {
			if (task_p->task_type == ISNS_TASK_SEND_PDU)
				isns_complete_trans(task_p->var.send_pdu.trans_p);
			isns_free_task(task_p);
		}
		/* Empty the task queue of any pending tasks and free them. */
		while ((task_p = isns_taskq_remove(cfg_p)) != NULL) {
			if (task_p->task_type == ISNS_TASK_SEND_PDU)
				isns_complete_trans(task_p->var.send_pdu.trans_p);
			isns_free_task(task_p);
		}
		pthread_mutex_destroy(&cfg_p->taskq_mutex);
		pthread_mutex_destroy(&cfg_p->trans_mutex);
		if (cfg_p->ai_p != NULL) {
			if (cfg_p->ai_p->ai_canonname != NULL)
				isns_free(cfg_p->ai_p->ai_canonname);
			if (cfg_p->ai_p->ai_addr != NULL)
				isns_free(cfg_p->ai_p->ai_addr);
			isns_free(cfg_p->ai_p);
		}
		isns_free(cfg_p);
	}
}


/*
 * isns_thread_create()
 */
int
isns_thread_create(struct isns_config_s *cfg_p)
{
	char namebuf[ISNS_THREAD_MAX_NAMELEN];
	int error;
	pthread_attr_t attr;

	DBG("isns_thread_create: entered\n");

	strcpy(namebuf, "isns_control");
	error = pthread_attr_init(&attr);
	if (error != 0) {
		DBG("isns_thread_create: error on pthread_threadattr_init\n");
		return error;
	}

	error = pthread_attr_setname_np(&attr, namebuf, NULL);
	if (error != 0) {
		DBG("isns_thread_create: "
		    "error on pthread_threadattr_setname\n");
		pthread_attr_destroy(&attr);
		return error;
	}

	error = pthread_create(cfg_p->control_thread_p,
	    &attr, isns_control_thread, cfg_p);
	pthread_attr_destroy(&attr);

	if (error != 0) {
		DBG("isns_thread_create: error on pthread_thread_create\n");
		return error;
    	}

	return error;
}


/*
 * isns_thread_destroy()
 */
void
isns_thread_destroy(struct isns_config_s *cfg_p)
{
	int error;
	void *rv;

	DBG("isns_thread_destroy: entered\n");

	if ((cfg_p == NULL) || (cfg_p->control_thread_p == NULL))
		return;

	DBG("isns_thread_destroy: about to wait (join) on thread\n");
	error = pthread_join(*cfg_p->control_thread_p, &rv);
	if (error) {
		DBG("isns_thread_destroy: error on pthread_thread_join\n");
		return;
	}

	DBG("isns_thread_destroy: done waiting on thread\n");
}

/*
 *
 */
void
isns_process_connection_loss(struct isns_config_s *cfg_p)
{
	struct isns_trans_s *trans_p;
	struct isns_pdu_s *pdu_p, *free_pdu_p;


	DBG("isns_process_connection_loss: entered\n");

	if (cfg_p->curtask_p != NULL) {
		trans_p = cfg_p->curtask_p->var.send_pdu.trans_p;

		if (trans_p->disconnect_cnt == ISNS_MAX_DISCONNECTS_PER_TRANS) {
			isns_complete_trans(trans_p);
			isns_end_task(cfg_p->curtask_p);

			if (cfg_p->pdu_in_p != NULL) {
				isns_free_pdu(cfg_p->pdu_in_p);
				cfg_p->pdu_in_p = NULL;
			}
		} else {
			trans_p->disconnect_cnt++;

			if (trans_p->pdu_rsp_list != NULL) {
				pdu_p = trans_p->pdu_rsp_list;
				while (pdu_p != NULL) {
					free_pdu_p = pdu_p;
					pdu_p = pdu_p->next;
					isns_free_pdu(free_pdu_p);
				}
			}

			isns_taskq_insert_head(cfg_p, cfg_p->curtask_p);
			cfg_p->curtask_p = NULL;

			isns_issue_cmd(cfg_p, ISNS_CMD_PROCESS_TASKQ);
		}
	}	
}
