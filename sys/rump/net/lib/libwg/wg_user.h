/*	$NetBSD: wg_user.h,v 1.1 2020/08/26 16:03:42 riastradh Exp $	*/

/*
 * Copyright (C) Ryota Ozaki <ozaki.ryota@gmail.com>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

struct wg_user;
struct wg_softc;

/*
 * Defined in wg_user.c and called from if_wg.c.
 */
int	rumpuser_wg_create(const char *tun_name, struct wg_softc *,
	    struct wg_user **);
void	rumpuser_wg_destroy(struct wg_user *);

void	rumpuser_wg_send_user(struct wg_user *, struct iovec *, size_t);
int	rumpuser_wg_send_peer(struct wg_user *, struct sockaddr *,
	    struct iovec *, size_t);

int	rumpuser_wg_ioctl(struct wg_user *, u_long, void *, int);
int	rumpuser_wg_sock_bind(struct wg_user *, const uint16_t);

char *	rumpuser_wg_get_tunname(struct wg_user *);

/*
 * Defined in if_wg.c and called from wg_user.c.
 */
void	rumpkern_wg_recv_user(struct wg_softc *, struct iovec *, size_t);
void	rumpkern_wg_recv_peer(struct wg_softc *, struct iovec *, size_t);
