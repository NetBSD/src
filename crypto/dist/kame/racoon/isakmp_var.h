/*	$KAME: isakmp_var.h,v 1.20 2001/12/12 15:29:14 sakane Exp $	*/

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

#define PORT_ISAKMP 500

#define DEFAULT_NONCE_SIZE	16

typedef u_char cookie_t[8];
typedef u_char msgid_t[4];

typedef struct { /* i_cookie + r_cookie */
	cookie_t i_ck;
	cookie_t r_ck;
} isakmp_index;

struct isakmp_gen;
struct sched;

struct sockaddr;
struct ph1handle;
struct ph2handle;
struct remoteconf;
struct isakmp_gen;
struct ipsecdoi_pl_id;	/* XXX */
struct isakmp_pl_ke;	/* XXX */
struct isakmp_pl_nonce;	/* XXX */

extern int isakmp_handler __P((int));
extern int isakmp_ph1begin_i __P((struct remoteconf *, struct sockaddr *));

extern vchar_t *isakmp_parsewoh __P((int, struct isakmp_gen *, int));
extern vchar_t *isakmp_parse __P((vchar_t *));

extern int isakmp_init __P((void));
extern const char *isakmp_pindex __P((const isakmp_index *, const u_int32_t));
extern int isakmp_open __P((void));
extern void isakmp_close __P((void));
extern int isakmp_send __P((struct ph1handle *, vchar_t *));

extern void isakmp_ph1resend_stub __P((void *));
extern int isakmp_ph1resend __P((struct ph1handle *));
extern void isakmp_ph2resend_stub __P((void *));
extern int isakmp_ph2resend __P((struct ph2handle *));
extern void isakmp_ph1expire_stub __P((void *));
extern void isakmp_ph1expire __P((struct ph1handle *));
extern void isakmp_ph1delete_stub __P((void *));
extern void isakmp_ph1delete __P((struct ph1handle *));
extern void isakmp_ph2expire_stub __P((void *));
extern void isakmp_ph2expire __P((struct ph2handle *));
extern void isakmp_ph2delete_stub __P((void *));
extern void isakmp_ph2delete __P((struct ph2handle *));

extern int isakmp_post_acquire __P((struct ph2handle *));
extern int isakmp_post_getspi __P((struct ph2handle *));
extern void isakmp_chkph1there_stub __P((void *));
extern void isakmp_chkph1there __P((struct ph2handle *));

extern caddr_t isakmp_set_attr_v __P((caddr_t, int, caddr_t, int));
extern caddr_t isakmp_set_attr_l __P((caddr_t, int, u_int32_t));
extern vchar_t *isakmp_add_attr_v __P((vchar_t *, int, caddr_t, int));
extern vchar_t *isakmp_add_attr_l __P((vchar_t *, int, u_int32_t));

extern int isakmp_newcookie __P((caddr_t, struct sockaddr *, struct sockaddr *));

extern int isakmp_p2ph __P((vchar_t **, struct isakmp_gen *));

extern u_int32_t isakmp_newmsgid2 __P((struct ph1handle *));
extern caddr_t set_isakmp_header __P((vchar_t *, struct ph1handle *, int));
extern caddr_t set_isakmp_header2 __P((vchar_t *, struct ph2handle *, int));
extern caddr_t set_isakmp_payload __P((caddr_t, vchar_t *, int));

#ifdef HAVE_PRINT_ISAKMP_C
extern void isakmp_printpacket __P((vchar_t *, struct sockaddr *,
	struct sockaddr *, int));
#endif

extern int copy_ph1addresses __P(( struct ph1handle *,
	struct remoteconf *, struct sockaddr *, struct sockaddr *));
extern void log_ph1established __P((const struct ph1handle *));
