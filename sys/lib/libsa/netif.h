/*	$NetBSD: netif.h,v 1.8 2024/02/27 16:09:19 christos Exp $	*/

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
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

#ifndef __SYS_LIBNETBOOT_NETIF_H
#define __SYS_LIBNETBOOT_NETIF_H

#include "iodesc.h"

struct netif; /* forward */

struct netif_driver {
	char	*netif_bname;
	int	(*netif_match)(struct netif *, void *);
	int	(*netif_probe)(struct netif *, void *);
	void	(*netif_init)(struct iodesc *, void *);
	int	(*netif_get)(struct iodesc *, void *, size_t, saseconds_t);
	int	(*netif_put)(struct iodesc *, void *, size_t);
	void	(*netif_end)(struct netif *);
	struct	netif_dif *netif_ifs;
	int	netif_nifs;
};

struct netif_dif {
	int		dif_unit;
	int		dif_nsel;
	struct netif_stats *dif_stats;
	void		*dif_private;
	/* the following fields are used internally by the netif layer */
	u_long		dif_used;
};

struct netif_stats {
	int	collisions;
	int	collision_error;
	int	missed;
	int	sent;
	int	received;
	int	deferred;
	int	overflow;
};

struct netif {
	struct netif_driver	*nif_driver;
	int			nif_unit;
	int			nif_sel;
	void			*nif_devdata;
};

extern struct netif_driver	*netif_drivers[];	/* machdep */
extern int			n_netif_drivers;

extern int			netif_debug;

void		netif_init(void);
struct netif	*netif_select(void *);
int		netif_probe(struct netif *, void *);
void		netif_attach(struct netif *, struct iodesc *, void *);
void		netif_detach(struct netif *);

int		netif_open(void *);
int		netif_close(int);

#endif /* __SYS_LIBNETBOOT_NETIF_H */
