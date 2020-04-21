/*	$NetBSD: privsep.h,v 1.6.52.2 2020/04/21 19:37:54 martin Exp $	*/

/* Id: privsep.h,v 1.5 2005/06/07 12:22:11 fredsen Exp */

/*
 * Copyright (C) 2004 Emmanuel Dreyfus
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

#ifndef _PRIVSEP_H
#define _PRIVSEP_H

#define PRIVSEP_EAY_GET_PKCS1PRIVKEY	0x0801	/* admin_com_bufs follows */
#define PRIVSEP_SCRIPT_EXEC		0x0803	/* admin_com_bufs follows */
#define PRIVSEP_GETPSK			0x0804	/* admin_com_bufs follows */
#define PRIVSEP_XAUTH_LOGIN_SYSTEM	0x0805	/* admin_com_bufs follows */
#define PRIVSEP_ACCOUNTING_PAM		0x0806	/* admin_com_bufs follows */
#define PRIVSEP_XAUTH_LOGIN_PAM		0x0807	/* admin_com_bufs follows */
#define PRIVSEP_CLEANUP_PAM		0x0808	/* admin_com_bufs follows */
#define PRIVSEP_ACCOUNTING_SYSTEM	0x0809	/* admin_com_bufs follows */
#define PRIVSEP_SETSOCKOPTS		0x080A	/* admin_com_bufs follows */
#define PRIVSEP_BIND			0x080B	/* admin_com_bufs follows */
#define PRIVSEP_SOCKET			0x080C	/* admin_com_bufs follows */

#define PRIVSEP_NBUF_MAX 24
#define PRIVSEP_BUFLEN_MAX 4096
struct admin_com_bufs {
	size_t buflen[PRIVSEP_NBUF_MAX];
	/* Followed by the buffers */
};

struct privsep_com_msg {
	struct admin_com hdr;
	struct admin_com_bufs bufs;
};

int privsep_init __P((void));

vchar_t *privsep_eay_get_pkcs1privkey __P((char *));
int privsep_script_exec __P((char *, int, char * const *));
int privsep_setsockopt __P((int, int, int, const void *, socklen_t));
int privsep_socket __P((int, int, int));
int privsep_bind __P((int, const struct sockaddr *, socklen_t));
vchar_t *privsep_getpsk __P((const char *, const int));
int privsep_xauth_login_system __P((char *, char *));
#ifdef HAVE_LIBPAM
int privsep_accounting_pam __P((int, int));
int privsep_xauth_login_pam __P((int, struct sockaddr *, char *, char *));
void privsep_cleanup_pam __P((int));
#endif
int privsep_accounting_system __P((int, struct sockaddr *, char *, int));
#endif /* _PRIVSEP_H */
