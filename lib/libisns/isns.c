/*	$NetBSD: isns.c,v 1.1.1.1 2011/01/16 01:22:50 agc Exp $	*/

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
__RCSID("$NetBSD: isns.c,v 1.1.1.1 2011/01/16 01:22:50 agc Exp $");

/*
 * isns.c
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/uio.h>
#include <sys/poll.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "isns.h"
#include "isns_config.h"



/*
 * isns_init()
 */
int
isns_init(ISNS_HANDLE *isns_handle_p, int is_server)
{
	struct isns_config_s *cfg_p;
	int rval;

	*isns_handle_p = NULL;

	if ((cfg_p = isns_new_config()) == NULL) {
		DBG("isns_init: error on isns_new_config()\n");
		return ENOMEM;
	}

	cfg_p->is_server = is_server;
	cfg_p->curtask_p = NULL;

	if ((rval = pipe(cfg_p->pipe_fds)) != 0) {
		DBG("isns_init: error on wepe_sys_pipe()\n");
		isns_destroy_config(cfg_p);
		return rval;
	}

	if ((cfg_p->kq = kqueue()) == -1) {
		DBG("isns_init: error on kqueue()\n");
		isns_destroy_config(cfg_p);
		return -1;
	}

	rval = isns_change_kevent_list(cfg_p, (uintptr_t)cfg_p->pipe_fds[0],
	    EVFILT_READ, EV_ADD | EV_ENABLE, (int64_t)0,
	    (intptr_t)isns_kevent_pipe);
	if (rval == -1) {
		DBG("isns_init: error on isns_change_kevent_list() "
		    "for isns_kevent_pipe\n");
		isns_destroy_config(cfg_p);
		return rval;
	}

	isns_init_buffer_pool();
	rval = isns_add_buffer_pool(ISNS_BUF_SIZE, ISNS_BUF_COUNT);
	if (rval != 0) {
		DBG("isns_init: error on isns_init_buffer_pool()\n");
		isns_destroy_config(cfg_p);
		return rval;
	}
	rval = isns_add_buffer_pool((int)ISNS_SMALL_BUF_SIZE, ISNS_SMALL_BUF_COUNT);
	if (rval != 0) {
		DBG("isns_init: error on isns_init_buffer_pool() [small]\n");
		isns_destroy_config(cfg_p);
		isns_destroy_buffer_pool();
		return rval;
	}

	if ((rval = isns_thread_create(cfg_p)) != 0) {
		DBG("isns_init: error on isns_thread_create()\n");
		isns_destroy_config(cfg_p);
		isns_destroy_buffer_pool();
		return rval;
	}

	*isns_handle_p = (ISNS_HANDLE)cfg_p;

	return 0;
}


/*
 * isns_add_servercon()
 */
int
isns_add_servercon(ISNS_HANDLE isns_handle, int fd, struct addrinfo *ai)
{
	struct isns_config_s *cfg_p;
	struct isns_task_s *task_p;
	struct addrinfo *ai_p;
	size_t len;

	if (isns_handle == ISNS_INVALID_HANDLE)
		return EINVAL;

	cfg_p = (struct isns_config_s *)isns_handle;

	ai_p = (struct addrinfo *)isns_malloc(sizeof(struct addrinfo));
	if (ai_p == NULL)
		return ENOMEM;

	ai_p->ai_flags = ai->ai_flags;
	ai_p->ai_family = ai->ai_family;
	ai_p->ai_socktype = ai->ai_socktype;
	ai_p->ai_protocol = ai->ai_protocol;
	ai_p->ai_addrlen = ai->ai_addrlen;
	if (ai->ai_canonname != NULL) {
		len = strlen(ai->ai_canonname);
		ai_p->ai_canonname = (char *)isns_malloc(len + 1);
		if (ai_p->ai_canonname == NULL) {
			isns_free(ai_p);
			return ENOMEM;
		}
		memset(ai_p->ai_canonname, '\0', len + 1);
		strncpy(ai_p->ai_canonname, ai->ai_canonname, len);
	} else
		ai_p->ai_canonname = NULL;
	if (ai->ai_addr != NULL) { 
		ai_p->ai_addr = (struct sockaddr *)isns_malloc(ai_p->
		    ai_addrlen);
		if (ai_p->ai_addr == NULL) {
			if (ai_p->ai_canonname != NULL)
				isns_free(ai_p->ai_canonname);
			isns_free(ai_p);
			return ENOMEM;
		}
		memcpy(ai_p->ai_addr, ai->ai_addr, ai_p->ai_addrlen);
	} else
		ai_p->ai_addr = NULL;
	ai_p->ai_next = NULL;
	
	/* Build task and kick off task processing */
	task_p = isns_new_task(cfg_p, ISNS_TASK_INIT_SOCKET_IO, 1);
	task_p->var.init_socket_io.sd = fd;
	task_p->var.init_socket_io.ai_p = ai_p;

	isns_taskq_insert_head(cfg_p, task_p);
	isns_issue_cmd(cfg_p, ISNS_CMD_PROCESS_TASKQ);
	isns_wait_task(task_p, NULL);

	return 0;
}


/*
 * isns_init_reg_refresh()
 */
int
isns_init_reg_refresh(ISNS_HANDLE isns_handle, const char *node, int interval)
{
	struct isns_config_s *cfg_p;
	struct isns_task_s *task_p;
	struct isns_refresh_s *ref_p;

	if (isns_handle == ISNS_INVALID_HANDLE)
		return EINVAL;

	/* Build INIT_REFRESH task with info provided. */
	cfg_p = (struct isns_config_s *)isns_handle;
	task_p = isns_new_task(cfg_p, ISNS_TASK_INIT_REFRESH, 0);
	if (task_p == NULL)
		return ENOMEM;

	ref_p = (struct isns_refresh_s *)
	    isns_malloc(sizeof(struct isns_refresh_s));
	if (ref_p == NULL) {
		isns_free_task(task_p);
		return ENOMEM;
	}

	(void) snprintf(ref_p->node, sizeof(ref_p->node), "%.*s",
		(int)sizeof(ref_p->node)-1, node);
	ref_p->interval = interval;
	ref_p->trans_p = NULL;
	task_p->var.init_refresh.ref_p = ref_p;

	isns_taskq_insert_tail(cfg_p, task_p);
	isns_issue_cmd(cfg_p, ISNS_CMD_PROCESS_TASKQ);

	return 0;
}


/*
 * isns_stop()
 */
void isns_stop(ISNS_HANDLE isns_handle)
{
	struct isns_config_s *cfg_p = (struct isns_config_s *)isns_handle;

	DBG("isns_stop: entered\n");

	isns_issue_cmd(cfg_p, ISNS_CMD_STOP);

	isns_thread_destroy(cfg_p);
	isns_destroy_config(cfg_p);
	isns_destroy_buffer_pool();
}
