/*	$NetBSD: isic_isapnp_itkix.c,v 1.1.4.2 2002/02/28 04:13:50 nathanw Exp $	*/

/*
 * Copyright (c) 2002 Martin Husemann <martin@duskware.de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: isic_isapnp_itkix.c,v 1.1.4.2 2002/02/28 04:13:50 nathanw Exp $");

#include "opt_isicpnp.h"

#ifdef ISICPNP_ITKIX

#include <sys/param.h>
#include <sys/ioccom.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/callout.h>
#include <machine/bus.h>
#include <sys/device.h>
#include <sys/socket.h>

#include <net/if.h>

#include <netisdn/i4b_debug.h>
#include <netisdn/i4b_ioctl.h>

#include <dev/ic/isic_l1.h>

/* XXX: this is a hack to share some more code - will be replaced with generic
 *      io-mapped isic driver */
#include "opt_isicisa.h"
#ifndef ISICISA_ITKIX1
#define ISICISA_ITKIX1
#include <dev/isa/isic_isa_itk_ix1.c>
#else
extern int isic_attach_itkix1(struct l1_softc *sc);
#endif

void isic_attach_isapnp_itkix1(struct l1_softc *sc);

void isic_attach_isapnp_itkix1(struct l1_softc *sc)
{
	isic_attach_itkix1(sc);
}

#endif
