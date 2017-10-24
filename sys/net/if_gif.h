/*	$NetBSD: if_gif.h,v 1.25.8.1 2017/10/24 08:47:24 snj Exp $	*/
/*	$KAME: if_gif.h,v 1.23 2001/07/27 09:21:42 itojun Exp $	*/

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

/*
 * if_gif.h
 */

#ifndef _NET_IF_GIF_H_
#define _NET_IF_GIF_H_

#include <sys/queue.h>
#include <sys/percpu.h>

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#endif

#include <netinet/in.h>
/* xxx sigh, why route have struct route instead of pointer? */

struct encaptab;

struct gif_ro {
	struct route gr_ro;
	kmutex_t gr_lock;
};

struct gif_softc {
	struct ifnet	gif_if;	   /* common area - must be at the top */
	struct sockaddr	*gif_psrc; /* Physical src addr */
	struct sockaddr	*gif_pdst; /* Physical dst addr */
	percpu_t *gif_ro_percpu;   /* struct gif_ro */
	int		gif_flags;
	const struct encaptab *encap_cookie4;
	const struct encaptab *encap_cookie6;
	LIST_ENTRY(gif_softc) gif_list;	/* list of all gifs */
};
#define GIF_ROUTE_TTL	10

#define GIF_MTU		(1280)	/* Default MTU */
#define	GIF_MTU_MIN	(1280)	/* Minimum MTU */
#define	GIF_MTU_MAX	(8192)	/* Maximum MTU */

/* Prototypes */
void	gif_input(struct mbuf *, int, struct ifnet *);

void	gif_rtcache_free_pc(void *, void *, struct cpu_info *);

#ifdef GIF_ENCAPCHECK
int	gif_encapcheck(struct mbuf *, int, int, void *);
#endif

/*
 * Locking notes:
 * + gif_softc_list is protected by gif_softcs.lock (an adaptive mutex)
 *       gif_softc_list is list of all gif_softcs. It is used by ioctl
 *       context only.
 * + Members of struct gif_softc except for gif_ro_percpu are protected by
 *   - encap_lock for writer
 *   - stopping processing when writer begin to run
 *     for reader(Tx and Rx processing)
 * + Each CPU's gif_ro.gr_ro of gif_ro_percpu are protected by
 *   percpu'ed gif_ro.gr_lock.
 *
 * Locking order:
 *     - encap_lock => gif_softcs.lock
 */
#endif /* !_NET_IF_GIF_H_ */
