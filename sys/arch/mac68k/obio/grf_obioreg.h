/* $NetBSD: grf_obioreg.h,v 1.1 2019/07/26 10:48:44 rin Exp $ */

/* NetBSD: grf_obio.c,v 1.58 2012/10/27 17:18:00 chs Exp */
/*
 * Copyright (C) 1998 Scott Reynolds
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
 * 3. The name of the author may not be used to endorse or promote products
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
 * Copyright (c) 1995 Allen Briggs.  All rights reserved.
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
 *	This product includes software developed by Allen Briggs.
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
 * DAFB framebuffer and register definitions
 */
#define	DAFB_FB_BASE		0xf9000000

#define	DAFB_CONTROL_BASE	0xf9800000

#define	DAFB_CMAP_BASE		0xf9800200	/* Taken from Linux */
#define	DAFB_CMAP_LEN		0x14
#define	DAFB_CMAP_RESET		0x00
#define	DAFB_CMAP_LUT		0x13

/*
 * Civic framebuffer and register definitions
 */
#define	CIVIC_FB_BASE		0x50100000

#define	CIVIC_CONTROL_BASE	0x50036000

#define	CIVIC_CMAP_BASE		0x50f30800	/* Taken from Linux */
#define	CIVIC_CMAP_LEN		0x30
#define	CIVIC_CMAP_ADDR		0x00
#define	CIVIC_CMAP_LUT		0x10
#define	CIVIC_CMAP_STATUS	0x20
#define	CIVIC_CMAP_VBLADDR	0x28
#define	CIVIC_CMAP_STATUS2	0x2c

/*
 * Valkyrie framebuffer and register definitions
 */
#define	VALKYRIE_FB_BASE	0xf9000000

#define	VALKYRIE_CONTROL_BASE	0x50f2a000

#define	VALKYRIE_CMAP_BASE	0x50f24000	/* Taken from Linux */
#define	VALKYRIE_CMAP_LEN	0x8
#define	VALKYRIE_CMAP_ADDR	0x0
#define	VALKYRIE_CMAP_LUT	0x4

/*
 * RBV register definitions
 */
#define	DAC_CMAP_BASE		0x50f24000	/* Taken from Linux */
#define	RBV_CMAP_BASE		DAC_CMAP_BASE
#define	RBV_CMAP_LEN		0xc
#define	RBV_CMAP_ADDR		0x0
#define	RBV_CMAP_LUT		0x4
#define	RBV_CMAP_CNTL		0x8
