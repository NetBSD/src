/* 	$NetBSD: xintcreg.h,v 1.1 2006/12/02 22:18:47 freza Exp $ */

/*
 * Copyright (c) 2006 Jachym Holecek
 * All rights reserved.
 *
 * Written for DFC Design, s.r.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
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

#ifndef	_VIRTEX_DEV_XINTCREG_H_
#define	_VIRTEX_DEV_XINTCREG_H_

#ifdef	_KERNEL_OPT
#include "opt_xintc.h"
#endif

#ifndef	DCR_XINTC_BASE
#error "XINTC component DCR base address undefined!"
#endif

/* Xilinx "XintC" interrupt controller, connects to DCR. */

#define	XINTC_ISR 	(DCR_XINTC_BASE + 0) 	/* Status (not masked) */
#define	XINTC_IPR 	(DCR_XINTC_BASE + 1) 	/* opt: Pending (masked) */
#define	XINTC_IER 	(DCR_XINTC_BASE + 2) 	/* Enable */
#define	XINTC_IAR 	(DCR_XINTC_BASE + 3) 	/* Acknowledge */
#define	XINTC_SIE 	(DCR_XINTC_BASE + 4) 	/* opt: Set Enable bits */
#define	XINTC_CIE 	(DCR_XINTC_BASE + 5) 	/* opt: Clr Enable bits */
#define	XINTC_IVR 	(DCR_XINTC_BASE + 6) 	/* opt: Vector */
#define	XINTC_MER 	(DCR_XINTC_BASE + 7) 	/* Master enable */

#define MER_ME 		0x00000001 		/* Master enable */
#define MER_HIE 	0x00000002 		/* Hw intr enable, write once */

#endif /*_VIRTEX_DEV_XINTCREG_H_*/
