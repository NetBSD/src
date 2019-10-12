/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2019 Roy Marples <roy@marples.name>
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

#include <stdbool.h>

#include "dhcpcd.h"

#if !defined(CTL_FREE_LIST)
#define	CTL_FREE_LIST 1
#elif CTL_FREE_LIST == 0
#undef	CTL_FREE_LIST
#endif

/* Limit queue size per fd */
#define CONTROL_QUEUE_MAX	100

struct fd_data {
	TAILQ_ENTRY(fd_data) next;
	void *data;
	size_t data_size;
	size_t data_len;
};
TAILQ_HEAD(fd_data_head, fd_data);

struct fd_list {
	TAILQ_ENTRY(fd_list) next;
	struct dhcpcd_ctx *ctx;
	int fd;
	unsigned int flags;
	struct fd_data_head queue;
	size_t queue_len;
#ifdef CTL_FREE_LIST
	struct fd_data_head free_queue;
#endif
};
TAILQ_HEAD(fd_list_head, fd_list);

#define FD_LISTEN	(1<<0)
#define FD_UNPRIV	(1<<1)

int control_start(struct dhcpcd_ctx *, const char *);
int control_stop(struct dhcpcd_ctx *);
int control_open(const char *);
ssize_t control_send(struct dhcpcd_ctx *, int, char * const *);
int control_queue(struct fd_list *, void *, size_t, bool);
void control_close(struct dhcpcd_ctx *ctx);

#endif
