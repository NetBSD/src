/*	$NetBSD: stic.c,v 1.2 1999/01/16 06:36:42 nisimura Exp $	*/

/*
 * Copyright (c) 1997 Jonathan Stone
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Jonathan Stone for
 *      the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

/*
 * Some portions derived from Mach: ga_hdw.c 2.5  93/05/17  15:16:52  rvb
 */

/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 * HISTORY
 * $Log: stic.c,v $
 * Revision 1.2  1999/01/16 06:36:42  nisimura
 * - Don't use void pointer for arithmetic.
 *
 * Revision 1.1  1997/11/11 04:47:57  jonathan
 * chipset driver for DEC pixelstamp  and STIC (stamp Interface chip).
 *
 * Revision 2.3  93/02/05  08:06:14  danner
 * 	Mods to *compile* on Flamingo.
 * 	[93/02/04  01:51:39  af]
 * 
 * Revision 2.2  92/05/22  15:47:02  jfriedl
 * 	Disable interrupts on exit.
 * 	[92/05/20  22:53:19  af]
 * 
 * 	Created, with some Ultrix code.
 * 	[92/05/09            af]
 * 
 */
/*
 *	File: ga_misc.c
 * 	Author: Alessandro Forin, Carnegie Mellon University
 *	Date:	5/92
 *
 *	Driver for the PMAG-CA 2D graphic board.
 *
 */

/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: stic.c,v 1.2 1999/01/16 06:36:42 nisimura Exp $");


/*
 * Support for the DEC pixelstamp and stamp asic (STIC)
 * chips used on 2-D (PMAG-C) and 3-D (PMAG-D, PMAG-E, PMAG-F)
 * DEC turbochannel option cards.
 */

#include <sys/param.h>
#include <sys/systm.h>

#include <dev/tc/tcvar.h>
#include <dev/tc/sticvar.h>

#include <machine/cpu.h>
#include <machine/bus.h>

int	stic_init __P((struct stic_softc *stic_sc));

int
stic_init(stic_sc)
	struct stic_softc *stic_sc;
{
	int modtype, xconfig, yconfig, config;
	volatile struct stic_regs *stic = STICADDR(stic_sc->stic_addr);
	caddr_t stamp_addr = stic_sc->stamp_addr;

	/*
	 *  Initialize STIC interface chip registers.
	 *  magic sequence from logic analyser.
	 */
	stic->sticsr = 0x00000030;	/* Get the STIC's attention. */
	tc_wmb();
	DELAY(4000);			/* wait 4ms for STIC to respond. */
	stic->sticsr = 0x00000000;	/* Hit the STIC's csr again... */
	stic->buscsr = 0xffffffff;	/* and bash its bus-acess csr. */
	tc_wmb();			/* Blam! */

	DELAY(20000);			/* wait until the stic recovers... */

#ifdef notyet
	/* init vdac */
	bt459_init(stic_sc->vdac_addr);
#endif

	/*
	 * the following code is taken from  Mach 3.0 MK85 source.
	 */

	/*
	 *  Initialize Stamp config register for model 0.
	 */
	modtype = stic->modcl;
	xconfig = (modtype & 0x800) >> 11;
	yconfig = (modtype & 0x600) >> 9;
	config = (yconfig << 1) | xconfig;

	/* stamp0 config */
	*(int32_t *)(stamp_addr+0x000b0) = config;
	*(int32_t *)(stamp_addr+0x000b4) = 0x0;

	if (yconfig > 0) {
		/*
		 * pixelstamp v1 configuration
		 */
		*(int *)(stamp_addr+0x100b0) = 0x8|config;
		*(int *)(stamp_addr+0x100b4) = 0x0;
		if (yconfig > 1) {
			/* pixelstamp v2 & v3 config */
		}
	}

    /*
     * Initialize STIC video registers.
     * (if we knew what we were doing, we might be able to frob this
     * to work at different montior frequencies.)
     */
    stic->vblank = (1024 << 16) | 1063;
    stic->vsync = (1027 << 16) | 1030;
    stic->hblank = (255 << 16) | 340;
    stic->hsync2 = 245;
    stic->hsync = (261 << 16) | 293;

    tc_wmb();	/* wbflush() */

    stic->ipdvint = STIC_INT_CLR;
    stic->sticsr = 0x00000008;
    tc_wmb();	/* wbflush() */
    return (0);
}
