/*	$NetBSD: brhreg.h,v 1.1 2003/01/25 02:00:17 thorpej Exp $	*/

/*
 * Copyright (c) 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _BRHREG_H_
#define	_BRHREG_H_

/*
 * Memory map and register definitions for the ADI Engineering
 * Big Red Head Evaluation Board.
 */

/*
 * This is arranged so that we can just use section mappings for
 * everything.  Note the physical size of the range may be larger
 * or smaller than the virtual size.
 *
 * We need to leave room near the top of the addres space for the
 * vectors.
 */

#define	BRH_PCI_CONF_VBASE	0xf9000000
#define	BRH_PCI_CONF_VSIZE	0x02000000	/* 32M */

#define	BRH_PCI_MEM1_VBASE	0xfb000000
#define	BRH_PCI_MEM1_VSIZE	0x02000000	/* 32M */

#define	BRH_PCI_MEM2_VBASE	0xfd000000
#define	BRH_PCI_MEM2_VSIZE	0x02000000	/* 32M */

#define	BRH_UART1_BASE		0x03000000
#define	BRH_UART1_VBASE		0xff000000
#define	BRH_UART1_VSIZE		0x00100000	/* 1M */

#define	BRH_UART2_BASE		0x03100000
#define	BRH_UART2_VBASE		0xff100000
#define	BRH_UART2_VSIZE		0x00100000	/* 1M */

#define	BRH_LED_BASE		0x03200000
#define	BRH_LED_VBASE		0xff200000
#define	BRH_LED_VSIZE		0x00100000	/* 1M */

#define	BRH_PCI_IO_VBASE	0xff300000
#define	BRH_PCI_IO_VSIZE	0x00100000	/* 1M */

#define	BRH_BECC_VBASE		0xff400000
#define	BRH_BECC_VSIZE		0x00100000	/* 1M */

	/*			0xffff0000	high vectors */

/*
 * The 7-segment display looks like so:
 *
 *         A
 *	+-----+
 *	|     |
 *    F	|     | B
 *	|  G  |
 *	+-----+
 *	|     |
 *    E	|     | C
 *	|  D  |
 *	+-----+ o  DP
 *
 * Setting a bit clears the corresponding segment on the
 * display.
 */
#define	SEG_A			(1 << 0)
#define	SEG_B			(1 << 1)
#define	SEG_C			(1 << 2)
#define	SEG_D			(1 << 3)
#define	SEG_E			(1 << 4)
#define	SEG_F			(1 << 5)
#define	SEG_G			(1 << 6)
#define	SEG_DP			(1 << 7)

#endif /* _BRHREG_H_ */
