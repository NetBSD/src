/*	$NetBSD: af_inet6.h,v 1.4.2.1 2008/06/23 04:29:57 wrstuden Exp $	*/

/*
 * Copyright (c) 1983, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* XXX */
#include <netinet/in.h>

#include "parse.h"

/* XXX */
extern struct in6_ifreq    in6_ridreq;
extern struct in6_aliasreq in6_addreq;

extern struct pkw ia6flags, inet6;

int	setia6flags(prop_dictionary_t, prop_dictionary_t);
int	setia6pltime(prop_dictionary_t, prop_dictionary_t);
int	setia6vltime(prop_dictionary_t, prop_dictionary_t);
int	setia6eui64(prop_dictionary_t, prop_dictionary_t);

void	in6_fillscopeid(struct sockaddr_in6 *sin6);
void	in6_status(prop_dictionary_t, prop_dictionary_t, bool);
void	in6_getaddr(const struct paddr_prefix *, int);
void	in6_init(void);
int	setia6eui64_impl(prop_dictionary_t, struct in6_aliasreq *);
int	setia6vltime_impl(prop_dictionary_t, struct in6_aliasreq *);
int	setia6pltime_impl(prop_dictionary_t, struct in6_aliasreq *);
int	setia6flags_impl(prop_dictionary_t, struct in6_aliasreq *);
void	in6_commit_address(prop_dictionary_t, prop_dictionary_t);
