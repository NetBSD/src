/*	$NetBSD: isns.h,v 1.2 2011/10/25 00:02:30 christos Exp $	*/

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
 * isns.h
 */

#ifndef _ISNS_H_
#define _ISNS_H_

#include <sys/types.h>
#include <sys/time.h>

#include <netdb.h>
#include <stdlib.h>

typedef void (*ISNS_HANDLE);
typedef void (*ISNS_TRANS);

#define ISNS_INVALID_HANDLE	(ISNS_HANDLE)NULL
#define ISNS_INVALID_TRANS	(ISNS_TRANS)NULL

#define ISNS_TLV_FIRST		0
#define ISNS_TLV_NEXT		1

int isns_init(ISNS_HANDLE *, int);
int isns_add_servercon(ISNS_HANDLE, int, struct addrinfo *);
int isns_init_reg_refresh(ISNS_HANDLE, const char *, int);
void isns_stop(ISNS_HANDLE);

ISNS_TRANS isns_new_trans(ISNS_HANDLE, uint16_t, uint16_t);
void isns_free_trans(ISNS_TRANS);
int isns_send_trans(ISNS_TRANS, const struct timespec *, uint32_t *);
int isns_add_tlv(ISNS_TRANS, uint32_t, uint32_t, const void *);
int isns_get_tlv(ISNS_TRANS, int, uint32_t *, uint32_t *, void **);

int	isns_add_string(ISNS_TRANS, uint32_t, const char *);

#endif /* !_ISNS_H_ */
