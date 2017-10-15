/*	$NetBSD: npfd.h,v 1.5 2017/10/15 15:26:10 christos Exp $	*/

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mindaugas Rasiukevicius.
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

#ifndef _NPFD_H_
#define _NPFD_H_

#define	PCAP_NPACKETS		1024
#define	NPFD_LOG_PATH		"/var/log"
#define NPF_DEV_PATH		"/dev/npf"

#define	NPFD_NPFLOG		"npflog"
#define	NPFD_NPFLOG_LEN		(sizeof(NPFD_NPFLOG) - 1)

struct npf_log;
typedef struct npfd_log npfd_log_t;

npfd_log_t *	npfd_log_create(const char *, const char *, const char *, int);
void		npfd_log_destroy(npfd_log_t *);
int		npfd_log_getsock(npfd_log_t *);
bool		npfd_log_file_reopen(npfd_log_t *, bool);
bool		npfd_log_pcap_reopen(npfd_log_t *);
int		npfd_log(npfd_log_t *);
void		npfd_log_stats(npfd_log_t *);
void		npfd_log_flush(npfd_log_t *);


#endif
