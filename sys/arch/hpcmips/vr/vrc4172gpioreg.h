/*	$NetBSD: vrc4172gpioreg.h,v 1.2 2001/05/06 14:25:16 takemura Exp $	*/

/*
 * Copyright (c) 2000 SATO Kazumi.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 *	Vrc4172 GPIO (General Purpose I/O) Unit Registers.
 */
#define VRC2_EXGPREG_MAX	0x4a
#define VRC2_EXGP_NPORTS	24
#define VRC2_EXGP_OFFSET	0x40

#define VRC2_EXGPDATA		VRC2_EXGPDATA0
#define VRC2_EXGPDIR		VRC2_EXGPDIR0
#define VRC2_EXGPINTEN		VRC2_EXGPINTEN0
#define VRC2_EXGPINTST		VRC2_EXGPINTST0
#define VRC2_EXGPINTTYP		VRC2_EXGPINTTYP0

#define VRC2_EXGPDATA0		0x00	/* I/O data (0..15) */
#define VRC2_EXGPDIR0		0x02	/* direction (0..15) */
#define VRC2_EXGPINTEN0		0x04	/* interrupt enable (0..15) */
#define VRC2_EXGPINTST0		0x06	/* interrupt status (0..15) */
#define VRC2_EXGPINTTYP0	0x08	/* interrupt type (0..15) */
#define VRC2_EXGPINTLV0L	0x0a	/* interrupt level low (0..15) */
#define VRC2_EXGPINTLV0H	0x0c	/* interrupt level high (0..15) */

#define VRC2_EXGPDATA1		0x40	/* I/O data (16..23) */
#define VRC2_EXGPDIR1		0x42	/* direction (16..23) */
#define VRC2_EXGPINTEN1		0x44	/* interrupt enable (16..23) */
#define VRC2_EXGPINTST1		0x46	/* interrupt status (16..23) */
#define VRC2_EXGPINTTYP1	0x48	/* interrupt type (16..23) */
#define VRC2_EXGPINTLV1L	0x4a	/* interrupt level low (16..23) */

#define VRC2_EXGPINTTYP_EDGE	1
#define VRC2_EXGPINTTYP_LEVEL	0
#define VRC2_EXGPINTAL_HIGH	1
#define VRC2_EXGPINTAL_LOW	0
#define VRC2_EXGPINTHT_HOLD	1
#define VRC2_EXGPINTHT_THROUGH	0

/* end */
