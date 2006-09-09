/*	$NetBSD: pfkey.h,v 1.4 2006/09/09 16:22:10 manu Exp $	*/

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
	u_int8_t	ps_satype;
	const char	*ps_name;
};

extern const struct pfkey_satype pfkey_satypes[];
extern const int pfkey_nsatypes;

extern int pfkey_handler __P((void));
extern vchar_t *pfkey_dump_sadb __P((int));
extern void pfkey_flush_sadb __P((u_int));
extern int pfkey_init __P((void));

extern struct pfkey_st *pfkey_getpst __P((caddr_t *, int, int));

extern int pk_checkalg __P((int, int, int));

struct ph2handle;
extern int pk_sendgetspi __P((struct ph2handle *));
extern int pk_sendupdate __P((struct ph2handle *));
extern int pk_sendadd __P((struct ph2handle *));
extern int pk_sendeacquire __P((struct ph2handle *));
extern int pk_sendspdupdate2 __P((struct ph2handle *));
extern int pk_sendspdadd2 __P((struct ph2handle *));
extern int pk_sendspddelete __P((struct ph2handle *));

extern void pfkey_timeover_stub __P((void *));
extern void pfkey_timeover __P((struct ph2handle *));

extern u_int pfkey2ipsecdoi_proto __P((u_int));
extern u_int ipsecdoi2pfkey_proto __P((u_int));
extern u_int pfkey2ipsecdoi_mode __P((u_int));
extern u_int ipsecdoi2pfkey_mode __P((u_int));

extern int pfkey_convertfromipsecdoi __P(( u_int, u_int, u_int,
	u_int *, u_int *, u_int *, u_int *, u_int *));
extern u_int32_t pk_getseq __P((void));
extern const char *sadbsecas2str
	__P((struct sockaddr *, struct sockaddr *, int, u_int32_t, int));

#endif /* _PFKEY_H */
