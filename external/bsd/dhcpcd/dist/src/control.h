/*
 * dhcpcd - DHCP client daemon
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2006-2025 Roy Marples <roy@marples.name>
 * All rights reserved

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef CONTROL_H
#define CONTROL_H

#include <sys/socket.h>

#include <stdbool.h>

#include "queue.h"

#if !defined(CTL_FREE_LIST)
#define CTL_FREE_LIST 1
#elif CTL_FREE_LIST == 0
#undef CTL_FREE_LIST
#endif

/* Maximum number of control socket users */
#define CONTROL_PEER_MAX 10

/* Limit queue size per fd */
#define CONTROL_QUEUE_MAX 100

struct fd_data {
	TAILQ_ENTRY(fd_data) next;
	void *data;
	size_t data_size;
	size_t data_len;
	unsigned int data_flags;
#define FD_DATA_SENDLEN 0x01
#ifdef PRIVSEP
	unsigned int data_peer_id;
#endif
};
TAILQ_HEAD(fd_data_head, fd_data);

struct fd_list {
	TAILQ_ENTRY(fd_list) next;
	struct dhcpcd_ctx *ctx;
	int fd;
	unsigned int flags;
	struct fd_data_head queue;
#ifdef CTL_FREE_LIST
	struct fd_data_head free_queue;
#endif
#ifdef PRIVSEP
	unsigned int id;
	unsigned int peer_id;
#endif
};
TAILQ_HEAD(fd_list_head, fd_list);

#define FD_READ	   0x01U
#define FD_CONTROL 0x02U
#define FD_COMMAND 0x04U
#define FD_LISTEN  0x08U

int control_start(struct dhcpcd_ctx *, const char *, sa_family_t);
int control_stop(struct dhcpcd_ctx *);
int control_open(const char *, sa_family_t);
ssize_t control_send(struct dhcpcd_ctx *, int, char *const *);
struct fd_list *control_find(struct dhcpcd_ctx *, int);
struct fd_list *control_new(struct dhcpcd_ctx *, int, unsigned int);
void control_free(struct fd_list *);
void control_delete(struct fd_list *);
ssize_t control_queuef(struct fd_list *, const void *, size_t, unsigned int);
ssize_t control_queue(struct fd_list *, const void *, size_t);
int control_recvmsg(struct fd_list *, struct msghdr *, size_t);
int control_user_ingroup(uid_t, gid_t, gid_t);
ssize_t control_handle_listen(struct fd_list *);
#endif
