/*	$NetBSD: isns_task.c,v 1.1.1.1 2011/01/16 01:22:50 agc Exp $	*/

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

#include <sys/cdefs.h>
__RCSID("$NetBSD: isns_task.c,v 1.1.1.1 2011/01/16 01:22:50 agc Exp $");

/*
 * isns_task.c
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "isns.h"
#include "isns_config.h"

static struct iovec write_buf[2 + (ISNS_MAX_PDU_PAYLOAD / ISNS_BUF_SIZE) +
    ((ISNS_MAX_PDU_PAYLOAD % ISNS_BUF_SIZE) != 0)];

static isns_task_handler isns_task_discover_server;
static isns_task_handler isns_task_reconnect_server;
static isns_task_handler isns_task_send_pdu;
static isns_task_handler isns_task_init_socket_io;
static isns_task_handler isns_task_init_refresh;


void
isns_run_task(struct isns_task_s *task_p)
{
	static isns_task_handler *task_dispatch_table[ISNS_NUM_TASKS] = {
		isns_task_discover_server,
		isns_task_reconnect_server,
		isns_task_send_pdu,
		isns_task_init_socket_io,
		isns_task_init_refresh
	};

	DBG("isns_run_task: task_type=%d\n", task_p->task_type);

	if (task_p->task_type < ARRAY_ELEMS(task_dispatch_table))
		task_dispatch_table[task_p->task_type](task_p);
	else
		DBG("isns_run_task: unknown task type=%d\n", task_p->task_type);
}


int
isns_wait_task(struct isns_task_s *task_p, const struct timespec *timeout_p)
{
	struct timeval tv_now;
	struct timespec ts_abstime;
	int rval;

	DBG("isns_wait_task: waitable=%d\n", task_p->waitable);

	if (!task_p->waitable)
		return EPERM;

	pthread_mutex_lock(&task_p->wait_mutex);

	if (timeout_p == NULL) {
		rval = pthread_cond_wait(&task_p->wait_condvar,
		    &task_p->wait_mutex);
	} else {
		gettimeofday(&tv_now, NULL);
		TIMEVAL_TO_TIMESPEC(&tv_now, &ts_abstime);
		timespecadd(&ts_abstime, timeout_p, &ts_abstime);

		rval = pthread_cond_timedwait(&task_p->wait_condvar,
		    &task_p->wait_mutex, &ts_abstime);
	}

	pthread_mutex_unlock(&task_p->wait_mutex);

	isns_free_task(task_p);

	DBG("isns_wait_task: wait done (rval=%d)\n", rval);

	return rval;
}


void
isns_end_task(struct isns_task_s *task_p)
{
	DBG("isns_end_task: %p\n", task_p);
	if (task_p == task_p->cfg_p->curtask_p)
		task_p->cfg_p->curtask_p = NULL;

	if (task_p->waitable)
		pthread_cond_signal(&task_p->wait_condvar);

	isns_free_task(task_p);
}


static void
isns_task_discover_server(struct isns_task_s *task_p)
{
	/* discover server here */
	DBG("isns_task_discover_server: entered\n");

	isns_end_task(task_p);
}


/*
 * isns_task_reconnect_server()
 */
static void
isns_task_reconnect_server(struct isns_task_s *task_p)
{
	struct addrinfo *ai_p;
	int rv;


	DBG("isns_task_reconnect_server: entered\n");
	
	ai_p = task_p->var.reconnect_server.ai_p;

	rv = isns_socket_create(&(task_p->cfg_p->sd), ai_p->ai_family,
	    ai_p->ai_socktype);
	if (rv != 0)
		return;

	rv = isns_socket_connect(task_p->cfg_p->sd, ai_p->ai_addr,
	    ai_p->ai_addrlen);
	if (rv != 0) {
		/* Add ISNS_EVT_TIMER_RECON to kqueue */
		rv = isns_change_kevent_list(task_p->cfg_p,
		    (uintptr_t)ISNS_EVT_TIMER_RECON, EVFILT_TIMER, EV_ADD,
		    (int64_t)ISNS_EVT_TIMER_RECON_PERIOD_MS,
		    (intptr_t)isns_kevent_timer_recon);
		if (rv == -1)
			DBG("isns_task_reconnect_server: error on "
			    "isns_change_kevent_list(1)\n");
	} else {
		task_p->cfg_p->sd_connected = 1;

		/* Add cfg_p->sd to kqueue */
		rv = isns_change_kevent_list(task_p->cfg_p,
		    (uintptr_t)(task_p->cfg_p->sd), EVFILT_READ,
		    EV_ADD | EV_CLEAR, (int64_t)0,
		    (intptr_t)isns_kevent_socket);
		if (rv == -1)
			DBG("isns_task_reconnect_server: error on "
			    "isns_change_kevent_lists(2)\n");

		isns_end_task(task_p);
	}
}

/*
 * isns_task_send_pdu()
 *
 * We send all of the pdu's associated with transaction task_p->trans_p here.
 *
 * Assumptions:
 *	(1) task_p->trans_p->pdu_req_list is an ordered (seq_id) list of
 *	    related (trans_id), appropriately sized pdus to be sent. The first
 *	    pdu has flag ISNS_FLAG_FIRST_PDU set and the last pdu has flag
 *	    ISNS_FLAG_LAST_PDU set.
 */
static void
isns_task_send_pdu(struct isns_task_s *task_p)
{
	struct iovec *iovp;
	struct isns_config_s *cfg_p;
	struct isns_pdu_s *pdu_p; /* points to first pdu in pdu_req_list */
	struct isns_buffer_s *buf_p;
	ssize_t bytes_written;
	ssize_t count;
	size_t bytes_to_write;
	int iovcnt, cur_iovec;
	char *ptr;


	DBG("isns_task_send_pdu: entered\n");

	cfg_p = task_p->cfg_p;
	pdu_p = task_p->var.send_pdu.pdu_p;

	while (pdu_p != NULL) {
		/* adjust byte order if necessary */
		if (pdu_p->byteorder_host) {
			pdu_p->hdr.isnsp_version = isns_htons(pdu_p->hdr.
			    isnsp_version);
			pdu_p->hdr.func_id = isns_htons(pdu_p->hdr.func_id);
			pdu_p->hdr.payload_len = isns_htons(pdu_p->hdr.
			    payload_len);
			pdu_p->hdr.flags = isns_htons(pdu_p->hdr.flags);
			pdu_p->hdr.trans_id = isns_htons(pdu_p->hdr.trans_id);
			pdu_p->hdr.seq_id = isns_htons(pdu_p->hdr.seq_id);

			pdu_p->byteorder_host = 0;
		}
		DUMP_PDU(pdu_p);

		/* send PDU via socket here */
		write_buf[0].iov_base = &(pdu_p->hdr);
		write_buf[0].iov_len = sizeof(pdu_p->hdr);
		bytes_to_write = write_buf[0].iov_len;
		iovcnt = 1;

		buf_p = pdu_p->payload_p;
		while (buf_p != NULL) {
			write_buf[iovcnt].iov_base = isns_buffer_data(buf_p,0);
			write_buf[iovcnt].iov_len = buf_p->cur_len;
			bytes_to_write += write_buf[iovcnt].iov_len;
			iovcnt++;
			buf_p = buf_p->next;
		}

		/* iovcnt and bytes_to_write are initialized */ 
		cur_iovec = 0;
		buf_p = ((struct isns_buffer_s *)(void *)pdu_p) - 1;
		do {
			iovp = &(write_buf[cur_iovec]);
			bytes_written = isns_socket_writev(cfg_p->sd, iovp,
			    iovcnt);
			if (bytes_written == -1) {
				DBG("isns_task_send_pdu: error on "
			    	"isns_socket_writev\n");
				isns_socket_close(cfg_p->sd);
				cfg_p->sd_connected = 0;

				isns_process_connection_loss(cfg_p);

				if (cfg_p->pdu_in_p != NULL) {
					isns_free_pdu(cfg_p->pdu_in_p);
					cfg_p->pdu_in_p = NULL;
				}
			
				break;
			}

			if (bytes_written < (ssize_t)bytes_to_write) {
				count = bytes_written;
				while (buf_p != NULL) { /* -OR- while (1) */
					if ((unsigned)count >= write_buf[
					    cur_iovec].iov_len) {
						count -= write_buf[cur_iovec].
						    iov_len;
						if (cur_iovec == 0)
							buf_p = pdu_p->
							    payload_p; 
						else
							buf_p = buf_p->next;
						cur_iovec++;
						iovcnt--;

						if (count == 0) {
							/* Do another write */
							break;
						} else {
							/* Look at new iovec */ 
							continue;
						}
					} else {
						write_buf[cur_iovec].iov_len -=
						    count;

						ptr = (char *) write_buf[cur_iovec].iov_base;
						ptr += count;
						write_buf[cur_iovec].iov_base = ptr;

						/* Do another write */
						break;
					}
				}
			}

			bytes_to_write -= bytes_written;
		} while (bytes_to_write);

		pdu_p = pdu_p->next;
	}

	if (!task_p->waitable) {
		isns_complete_trans(task_p->var.send_pdu.trans_p);
		isns_end_task(task_p);
	}
}

/*
 * isns_task_init_socket_io()
 */
static void
isns_task_init_socket_io(struct isns_task_s *task_p)
{
	struct isns_config_s *cfg_p;
	int rv;


	DBG("isns_task_init_socket_io: entered\n");

	cfg_p = task_p->cfg_p;

	if (cfg_p->sd_connected) {
		isns_socket_close(cfg_p->sd);
		cfg_p->sd_connected = 0;

		/* We may have received part of an unsolicited/duplicate pdu */
		if (cfg_p->pdu_in_p != NULL) {
			isns_free_pdu(cfg_p->pdu_in_p);
			cfg_p->pdu_in_p = NULL;
		}
	}

	/* May have an allocated 'struct addrinfo', whether connected or not */
	if (cfg_p->ai_p != NULL) {
		isns_free(cfg_p->ai_p);
		cfg_p->ai_p = NULL;
	}

	cfg_p->sd = task_p->var.init_socket_io.sd;
	cfg_p->ai_p = task_p->var.init_socket_io.ai_p;

	cfg_p->sd_connected = 1;

	/* Add cfg_p->sd to kqueue */
	rv = isns_change_kevent_list(cfg_p, (uintptr_t)cfg_p->sd,
	    EVFILT_READ, EV_ADD | EV_CLEAR, (int64_t)0,
	    (intptr_t)isns_kevent_socket);
	if (rv == -1)
		DBG("isns_task_init_socket_io: error on "
		    "isns_change_kevent_list\n");

	isns_end_task(task_p);
}


/*
 * isns_task_init_refresh(struct isns_task_s *task_p)
 */
static void
isns_task_init_refresh(struct isns_task_s *task_p)
{
	struct isns_config_s *cfg_p;
	int rval;

	DBG("isns_task_init_refresh: entered\n");

	/* Free any previous refresh info. */
	cfg_p = task_p->cfg_p;
	if (cfg_p->refresh_p != NULL) {
		if (cfg_p->refresh_p->trans_p != NULL)
			isns_free_trans(cfg_p->refresh_p->trans_p);
		isns_free(cfg_p->refresh_p);
	}

	/* Assign new refresh info into config struct. */
	cfg_p->refresh_p = task_p->var.init_refresh.ref_p;
	cfg_p->refresh_p->trans_p = NULL;

	/* Setup (or change) kevent timer for reg refresh. */
	rval = isns_change_kevent_list(cfg_p,
	    (uintptr_t)ISNS_EVT_TIMER_REFRESH, EVFILT_TIMER,
	    EV_ADD | EV_ENABLE, (int64_t)cfg_p->refresh_p->interval * 1000,
	    (intptr_t)isns_kevent_timer_refresh);
	if (rval == -1) {
		DBG("isns_task_init_refresh: "
		    "error on isns_change_kevent_list()\n");
	}

	isns_end_task(task_p);
}


struct isns_task_s *
isns_new_task(struct isns_config_s *cfg_p, uint8_t task_type, int waitable)
{
	struct isns_buffer_s *buf_p;
	struct isns_task_s *task_p;
	pthread_mutexattr_t mutexattr;
	pthread_condattr_t condattr;

	task_p = NULL;
	buf_p = isns_new_buffer((int)sizeof(struct isns_task_s));
	if (buf_p) {
		task_p = (struct isns_task_s *)isns_buffer_data(buf_p, 0);
		task_p->cfg_p = cfg_p;
		task_p->task_type = task_type;
		task_p->waitable = waitable;

		if (waitable) {
			pthread_mutexattr_init(&mutexattr);
			pthread_mutexattr_settype(&mutexattr,
			    ISNS_MUTEX_TYPE_NORMAL);
			pthread_mutex_init(&task_p->wait_mutex, &mutexattr);

			pthread_condattr_init(&condattr);
			pthread_cond_init(&task_p->wait_condvar, &condattr);
			task_p->wait_ref_count = 2;
		}
	}

	DBG("isns_new_task: %p, waitable=%d\n", task_p, waitable);

	return task_p;
}


void
isns_free_task(struct isns_task_s *task_p)
{
	struct isns_buffer_s *buf_p;
	int ref_count;

	DBG("isns_free_task: %p\n", task_p);
	if (task_p->waitable) {
		pthread_mutex_lock(&task_p->wait_mutex);
		ref_count = --task_p->wait_ref_count;
		pthread_mutex_unlock(&task_p->wait_mutex);

		if (ref_count > 0) {
			DBG("isns_free_task: ref_count > 0, no free done\n");
			return;
		}

		pthread_mutex_destroy(&task_p->wait_mutex);
		pthread_cond_destroy(&task_p->wait_condvar);
	}
	buf_p = ((struct isns_buffer_s *)(void *)(task_p))-1;
	isns_free_buffer(buf_p);
}


void
isns_taskq_insert_head(struct isns_config_s *cfg_p,
    struct isns_task_s *task_p)
{
	pthread_mutex_lock(&cfg_p->taskq_mutex);
	SIMPLEQ_INSERT_HEAD(&cfg_p->taskq_head, task_p, taskq_entry);
	pthread_mutex_unlock(&cfg_p->taskq_mutex);

	DBG("isns_taskq_insert_head: %p\n", task_p);
}


void
isns_taskq_insert_tail(struct isns_config_s *cfg_p,
    struct isns_task_s *task_p)
{
	pthread_mutex_lock(&cfg_p->taskq_mutex);
	SIMPLEQ_INSERT_TAIL(&cfg_p->taskq_head, task_p, taskq_entry);
	pthread_mutex_unlock(&cfg_p->taskq_mutex);

	DBG("isns_taskq_insert_tail: %p\n", task_p);
}


struct isns_task_s *
isns_taskq_remove(struct isns_config_s *cfg_p)
{
	struct isns_task_s *task_p = NULL;

	pthread_mutex_lock(&cfg_p->taskq_mutex);
	if ((task_p = SIMPLEQ_FIRST(&cfg_p->taskq_head)) != NULL)
		SIMPLEQ_REMOVE_HEAD(&cfg_p->taskq_head, taskq_entry);
	pthread_mutex_unlock(&cfg_p->taskq_mutex);

	DBG("isns_taskq_remove: %p\n", task_p);

	return task_p;
}


struct isns_task_s *
isns_taskq_remove_trans(struct isns_config_s *cfg_p, uint16_t trans_id)
{
	struct isns_task_s *task_p;
	int trans_found;

	trans_found = 0;
	pthread_mutex_lock(&cfg_p->taskq_mutex);
	SIMPLEQ_FOREACH(task_p, &cfg_p->taskq_head, taskq_entry) {
		if ((task_p->task_type == ISNS_TASK_SEND_PDU)
		    && (task_p->var.send_pdu.trans_p->id == trans_id)) {
			trans_found = 1;
			break;
		}
	}
	if (trans_found) {
		SIMPLEQ_REMOVE(&cfg_p->taskq_head, task_p, isns_task_s,
		    taskq_entry);
	}
	pthread_mutex_unlock(&cfg_p->taskq_mutex);

	return (trans_found ? task_p : NULL);
}
