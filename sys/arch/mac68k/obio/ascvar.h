/*	$NetBSD: ascvar.h,v 1.5.4.1 2000/06/30 16:27:28 simonb Exp $	*/

/*
 * Copyright (C) 1997 Scott Reynolds.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#include <sys/callout.h>

#define ASCUNIT(d)	((d) & 0x7)

struct asc_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_tag;
	bus_space_handle_t	sc_handle;
	int			sc_open;
	int			sc_ringing;
	struct callout		sc_bell_ch;
};

int	ascopen __P((dev_t dev, int flag, int mode, struct proc *p));
int	ascclose __P((dev_t dev, int flag, int mode, struct proc *p));
int	ascread __P((dev_t, struct uio *, int));
int	ascwrite __P((dev_t, struct uio *, int));
int	ascioctl __P((dev_t, int, caddr_t, int, struct proc *p));
int	ascpoll __P((dev_t dev, int events, struct proc *p));
paddr_t	ascmmap __P((dev_t dev, off_t off, int prot));
