/*	$NetBSD: hid.h,v 1.1 2000/05/25 22:12:00 is Exp $ */

/*
 * Copyright (C) 2000 Adam Ciarcinski.
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
 *	This product includes software developed by Adam Ciarcinski for
 *	the NetBSD project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* 603e/604e/750 HID0 */
#define	HID0_EMCP	0x80000000
#define	HID0_DBP	0x40000000
#define	HID0_EBA	0x20000000
#define	HID0_EBD	0x10000000
#define	HID0_BCLK	0x08000000
#define	HID0_EICE	0x04000000
#define	HID0_ECLK	0x02000000
#define	HID0_PAR	0x01000000
#define	HID0_DOZE	0x00800000
#define	HID0_NAP	0x00400000
#define	HID0_SLEEP	0x00200000
#define	HID0_DPM	0x00100000
#define	HID0_RISEG	0x00080000

#define	HID0_NHR	0x00010000
#define	HID0_ICE	0x00008000
#define	HID0_DCE	0x00004000
#define	HID0_ILOCK	0x00002000
#define	HID0_DLOCK	0x00001000
#define	HID0_ICFI	0x00000800
#define	HID0_DCFI	0x00000400
#define	HID0_SPD	0x00000200
#define	HID0_IFEM	0x00000100
#define	HID0_SGE	0x00000080
#define	HID0_DCFA	0x00000040
#define	HID0_BTIC	0x00000020
#define	HID0_FBIOB	0x00000010
#define	HID0_ABE	0x00000008
#define	HID0_BHT	0x00000004
#define	HID0_BTACD	0x00000002
#define	HID0_NOOPTI	0x00000001

/* 603e/604e/750 HID0 */
#define	HID0_EMCP	0x80000000
#define	HID0_DBP	0x40000000
#define	HID0_EBA	0x20000000
#define	HID0_EBD	0x10000000
#define	HID0_BCLK	0x08000000
#define	HID0_EICE	0x04000000
#define	HID0_ECLK	0x02000000
#define	HID0_PAR	0x01000000
#define	HID0_DOZE	0x00800000
#define	HID0_NAP	0x00400000
#define	HID0_SLEEP	0x00200000
#define	HID0_DPM	0x00100000
#define	HID0_RISEG	0x00080000

#define	HID0_NHR	0x00010000
#define	HID0_ICE	0x00008000
#define	HID0_DCE	0x00004000
#define	HID0_ILOCK	0x00002000
#define	HID0_DLOCK	0x00001000
#define	HID0_ICFI	0x00000800
#define	HID0_DCFI	0x00000400
#define	HID0_SPD	0x00000200
#define	HID0_IFEM	0x00000100
#define	HID0_SGE	0x00000080
#define	HID0_DCFA	0x00000040
#define	HID0_BTIC	0x00000020
#define	HID0_FBIOB	0x00000010
#define	HID0_ABE	0x00000008
#define	HID0_BHT	0x00000004
#define	HID0_BTACD	0x00000002
#define	HID0_NOOPTI	0x00000001
