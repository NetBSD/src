/*	$NetBSD: p4reg.h,v 1.5.76.1 2008/05/18 12:32:54 yamt Exp $	*/

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

/*
 * Copyright (c) 1995 Theo de Raadt
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

/*
 * P4 framebuffer register.  Most Sun framebuffers have a P4
 * register at the beginning of the physical space occupied
 * by the framebuffer that identifies its type, size, etc.
 */

#define P4_REG_DIAG		0x80
#define P4_REG_READBACKCLR	0x40
#define P4_REG_VIDEO		0x20
#define P4_REG_SYNC		0x10
#define P4_REG_VTRACE		0x08
#define	P4_REG_INT		0x04	/* r: pending */
#define	P4_REG_INTCLR		0x04	/* w: clear it */
#define	P4_REG_INTEN		0x02
#define P4_REG_FIRSTHALF	0x01
#define P4_REG_RESET		0x01

#define P4_FBTYPE_MASK	0x7f000000
#define	P4_FBTYPE(x)		((x) >> 24)

#define	P4_ID_MASK		0xf0
#define P4_ID(x)		(P4_FBTYPE((x)) == P4_ID_COLOR24 ? \
				    P4_ID_COLOR24 : \
				    P4_FBTYPE((x)) & P4_ID_MASK)

#define P4_NOTFOUND		-1	/* no P4 register */
#define P4_ID_BW		0x00	/* BW2: monochrome */
#define P4_ID_FASTCOLOR 	0x60	/* CG6: accelerated 8-bit color */
#define P4_ID_COLOR8P1		0x40	/* CG4: 8-bit color + overlay */
#define P4_ID_COLOR24		0x45	/* CG8: 24-bit color + overlay */

#define P4_SIZE_MASK		0x0f
#define P4_SIZE(x)		(P4_FBTYPE((x)) & P4_SIZE_MASK)
#define P4_SIZE_1600X1280	0x00
#define P4_SIZE_1152X900	0x01
#define P4_SIZE_1024X1024	0x02
#define P4_SIZE_1280X1024	0x03
#define P4_SIZE_1440X1440	0x04
#define P4_SIZE_640X480	0x05

/* Offset of bwtwo framebuffer from P4 register */
#define	P4_BW_OFF		0x00100000

/* Offsets for color framebuffers */
#define P4_COLOR_OFF_OVERLAY	0x00100000
#define P4_COLOR_OFF_ENABLE	0x00300000
#define P4_COLOR_OFF_COLOR	0x00500000
#define P4_COLOR_OFF_END	0x00700000
#define P4_COLOR_OFF_CMAP	0xfff00000	/* (-0x00100000) */
