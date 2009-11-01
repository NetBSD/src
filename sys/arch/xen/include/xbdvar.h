/* $NetBSD: xbdvar.h,v 1.11.18.1 2009/11/01 13:58:45 jym Exp $ */

/*
 *
 * Copyright (c) 2004 Christian Limpach.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef _XEN_XBDVAR_H_
#define _XEN_XBDVAR_H_

#include <sys/simplelock.h>

struct xbd_softc {
	device_t		sc_dev;		/* base device glue */
	struct dk_softc		sc_dksc;	/* generic disk interface */
	unsigned long		sc_xd_device;	/* cookie identifying device */
	struct dk_intf		*sc_di;		/* pseudo-disk interface */
	struct simplelock	sc_slock;	/* our lock */
	int			sc_shutdown;	/* about to be removed */
#if NRND > 0
	rndsource_element_t	sc_rnd_source;
#endif
};

struct xbd_attach_args {
	const char 		*xa_device;
	vdisk_t			*xa_xd;
	struct dk_intf		*xa_dkintf;
	struct sysctlnode	*xa_diskcookies;
};

int xbd_scan(device_t, struct xbd_attach_args *, cfprint_t);
void xbd_suspend(void);
void xbd_resume(void);

#endif /* _XEN_XBDVAR_H_ */
