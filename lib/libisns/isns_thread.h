/*	$NetBSD: isns_thread.h,v 1.1.1.1 2011/01/16 01:22:50 agc Exp $	*/

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
 * isns_thread.h
 */

#ifndef _ISNS_THREAD_H
#define _ISNS_THREAD_H

#include <sys/event.h>
#include <pthread.h>

#define ISNS_CMD_STOP			1
#define ISNS_CMD_PROCESS_TASKQ		2
#define ISNS_CMD_ABORT_TRANS		3

#define ISNS_EVT_TIMER_REFRESH		1
#define ISNS_EVT_TIMER_RECON 		2
#define ISNS_EVT_TIMER_RECON_PERIOD_MS 	1000

#define ISNS_THREAD_MAX_NAMELEN		128

typedef int (isns_kevent_handler)(struct kevent *, struct isns_config_s *);

extern void *isns_control_thread(void *arg);
extern isns_kevent_handler isns_kevent_pipe;
extern isns_kevent_handler isns_kevent_socket;
extern isns_kevent_handler isns_kevent_timer_recon;
extern isns_kevent_handler isns_kevent_timer_refresh;


#endif /* !_ISNS_THREAD_H */
