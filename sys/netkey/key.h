/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* $Id: key.h,v 1.1.2.1 1999/06/28 06:37:09 itojun Exp $ */

#ifndef _NETKEY_KEY_H_
#define _NETKEY_KEY_H_

#ifdef __NetBSD__
# ifdef _KERNEL
#  define KERNEL
# endif
#endif

#if defined(KERNEL)

extern struct key_cb key_cb;

struct sadb_address;
struct secindex;
struct secas;
struct sockaddr;
struct socket;
struct secexpire;
struct sadb_msg;
struct sadb_x_policy;
struct ipsecrequest;
extern struct secpolicy *key_allocsp __P((struct secindex *));
#if 0
extern int key_checkpolicy __P((struct secpolicy *));
#endif
extern int key_checkrequest __P((struct ipsecrequest *));
extern struct secas *key_allocsa __P((u_int, caddr_t, caddr_t, u_int, u_int));
extern void key_freesp __P((struct secpolicy *));
extern void key_freeso __P((struct socket *));
extern void key_freesa __P((struct secas *));

extern struct secpolicy *key_newsp __P((void));
extern struct secpolicy *key_msg2sp __P((struct sadb_x_policy *));
extern struct sadb_x_policy *key_sp2msg __P((struct secpolicy *));

extern int key_setsecidx __P((struct sadb_address *, struct sadb_address *,
				struct secindex *, int));
extern void key_delsecidx __P((struct secindex *));
extern int key_setexptime __P((struct secas *));
extern void key_timehandler __P((void));
extern void key_srandom __P((void));
extern int key_ismyaddr __P((u_int, caddr_t));
extern void key_freereg __P((struct socket *));
extern int key_parse __P((struct sadb_msg **, struct socket *, int *));
extern void key_init __P((void));
extern int key_checktunnelsanity __P((struct secas *, u_int, caddr_t, caddr_t));
extern void key_sa_recordxfer __P((struct secas *, struct mbuf *));
extern void key_sa_routechange __P((struct sockaddr *));
#ifdef MALLOC_DECLARE
MALLOC_DECLARE(M_SECA);
#endif /* MALLOC_DECLARE */

#if defined(__bsdi__) || defined(__NetBSD__)
extern int key_sysctl __P((int *, u_int, void *, size_t *, void *, size_t));
#endif

#endif /* defined(KERNEL) */

#endif /* _NETKEY_KEY_H_ */

