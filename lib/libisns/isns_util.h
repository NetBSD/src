/*	$NetBSD: isns_util.h,v 1.2 2011/08/17 10:08:43 christos Exp $	*/

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
 * isns_util.h
 */

#ifndef _ISNS_UTIL_H_
#define _ISNS_UTIL_H_

#define isns_ntohs	ntohs
#define isns_htons	htons
#define isns_ntohl	ntohl
#define isns_htonl	htonl

#define isns_malloc	malloc
#define isns_free	free


#define ARRAY_ELEMS(a)	(sizeof(a)/sizeof((a)[0]))


struct isns_config_s;
int isns_issue_cmd(struct isns_config_s *, uint8_t);
int isns_issue_cmd_with_data(struct isns_config_s *, uint8_t,
    uint8_t *, int);

int isns_change_kevent_list(struct isns_config_s *,
    uintptr_t, uint32_t, uint32_t, int64_t,
    intptr_t);

struct isns_config_s *isns_new_config(void);
void isns_destroy_config(struct isns_config_s *);

int isns_thread_create(struct isns_config_s *);
void isns_thread_destroy(struct isns_config_s *);

void isns_process_connection_loss(struct isns_config_s *);

#endif /* !_ISNS_UTIL_H_ */
