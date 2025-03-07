/*	$NetBSD: pfkey.h,v 1.9 2025/03/07 15:55:29 christos Exp $	*/

/* Id: pfkey.h,v 1.3 2004/06/11 16:00:17 ludvigm Exp */

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

#ifndef _PFKEY_H
#define _PFKEY_H

struct pfkey_satype {
	uint8_t	ps_satype;
	const char	*ps_name;
};

extern const struct pfkey_satype pfkey_satypes[];
extern const int pfkey_nsatypes;

extern vchar_t *pfkey_dump_sadb(int);
extern void pfkey_flush_sadb(u_int);
extern int pfkey_init(void);
extern int pfkey_reload(void);

extern struct pfkey_st *pfkey_getpst(caddr_t *, int, int);

extern int pk_checkalg(int, int, int);

struct ph2handle;
extern void pk_fixup_sa_addresses(caddr_t *mhp);
extern int pk_sendgetspi(struct ph2handle *);
extern int pk_sendupdate(struct ph2handle *);
extern int pk_sendadd(struct ph2handle *);
extern int pk_sendeacquire(struct ph2handle *);
extern int pk_sendspdupdate2(struct ph2handle *);
extern int pk_sendspdadd2(struct ph2handle *);
extern int pk_sendspddelete(struct ph2handle *);

extern u_int pfkey2ipsecdoi_proto(u_int);
extern u_int ipsecdoi2pfkey_proto(u_int);
extern u_int pfkey2ipsecdoi_mode(u_int);
extern u_int ipsecdoi2pfkey_mode(u_int);

extern int pfkey_convertfromipsecdoi(u_int, u_int, u_int,
    u_int *, u_int *, u_int *, u_int *, u_int *);
extern uint32_t pk_getseq(void);
extern const char *sadbsecas2str(struct sockaddr *, struct sockaddr *,
    int, uint32_t, int);

#endif /* _PFKEY_H */
