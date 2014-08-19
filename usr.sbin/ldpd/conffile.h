/* $NetBSD: conffile.h,v 1.1.12.2 2014/08/20 00:05:09 tls Exp $ */

/*
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mihai Chelaru <kefren@NetBSD.org>
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

#ifndef __CONFFILE_H
#define __CONFFILE_H

#include <net/if.h>
#include <netinet/in.h>
#include <sys/queue.h>

#define E_CONF_OK 0
#define	E_CONF_NOMATCH -1
#define E_CONF_AMBIGUOUS -2
#define E_CONF_IO -3
#define E_CONF_MEM -4
#define E_CONF_GENERIC -5
#define E_CONF_PARAM -6

struct conf_neighbour {
	struct in_addr address;
	int authenticate;	/* RFC 2385 */
	SLIST_ENTRY(conf_neighbour) neilist;
};
SLIST_HEAD(,conf_neighbour) conei_head;

struct conf_interface {
	char if_name[IF_NAMESIZE];
	struct in_addr tr_addr;
	int passive;
	SLIST_ENTRY(conf_interface) iflist;
};
SLIST_HEAD(,conf_interface) coifs_head;

int conf_parsefile(const char *fname);

#endif
