/*	$NetBSD: icsidereg.h,v 1.1 2001/10/05 22:27:56 reinoud Exp $	*/

/*
 * Copyright (c) 1997 Mark Brinicombe
 * Copyright (c) 1997 Causality Limited
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
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
 * Registers and address offsets for the ICS IDE card.
 */

/* ID register, read 4 consecutive words and extract ID from bit 0 */
#define ID_REGISTER_OFFSET		0x2280	/* byte offset from fast base */

#define REGISTER_SPACING_SHIFT		6
#define IDE_REGISTER_SPACE		0x200
#define AUX_REGISTER_SPACE		4
#define IRQ_REGISTER_SPACE		4
#define ID_REGISTER_SPACE		4
#define IRQ_STATUS_REGISTER_MASK	0x01

/* IDE drive registers */

/* ARCIN V5 registers */
#define V5_IDE_BASE			0x2800	/* byte offset from base */
#define V5_AUX_BASE			0x2a80	/* byte offset from base */
#define V5_IRQ_BASE			0x0004	/* byte offset from base */
#define V5_IRQSTAT_BASE			0x0000	/* byte offset from base */

/* ARCIN V6 registers */
#define V6_P_IDE_BASE			0x2000	/* byte offset from base */
#define V6_P_AUX_BASE			0x2380	/* byte offset from base */
#define V6_P_IRQ_BASE			0x2200	/* byte offset from base */
#define V6_P_IRQSTAT_BASE		0x2290	/* byte offset from base */

#define V6_S_IDE_BASE			0x3000	/* byte offset from base */
#define V6_S_AUX_BASE			0x3380	/* byte offset from base */
#define V6_S_IRQ_BASE			0x3200	/* byte offset from base */
#define V6_S_IRQSTAT_BASE		0x3290	/* byte offset from base */
