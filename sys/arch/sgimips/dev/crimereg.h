/*	$NetBSD: crimereg.h,v 1.2.10.1 2002/04/01 07:42:18 nathanw Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.netbsd.org/ for
 *          information about NetBSD.
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
 * O2 CRIME register definitions
 */

#define CRIME_REV		0x000
#define CRIME_CONTROL		0x008
#define CRIME_INTSTAT		0x010
#define CRIME_INTMASK		0x018
#define CRIME_SOFTINT		0x020
#define CRIME_HARDINT		0x028
#define CRIME_DOG		0x030
#define McGriff			CRIME_DOG	/* Baseball compatibility */
#define CRIME_TIME		0x038
#define CRIME_CPU_ERROR_ADDR	0x040
#define CRIME_CPU_ERROR_STAT	0x048
#define CRIME_CPU_ERROR_ENA	0x050
#define CRM_VICE_ERROR_ADDR	0x058
#define CRM_MEM_CONTROL		0x200
#define CRM_MEM_BANK_CTRL0	0x208
#define CRM_MEM_BANK_CTRL1	0x218
#define CRM_MEM_BANK_CTRL2	0x210
#define CRM_MEM_BANK_CTRL3	0x228
#define CRM_MEM_BANK_CTRL4	0x220
#define CRM_MEM_BANK_CTRL5	0x238
#define CRM_MEM_BANK_CTRL6	0x230
#define CRM_MEM_BANK_CTRL7	0x248
#define CRM_MEM_REFRESH_CNTR	0x248
#define CRM_MEM_ERROR_STAT	0x250
#define CRM_MEM_ERROR_ADDR	0x258
#define CRM_MEM_ERROR_ECC_SYN	0x260
#define CRM_MEM_ERROR_ECC_CHK	0x268
#define CRM_MEM_ERROR_ECC_REPL	0x270

#define CRM_INT_VICE		0x80000000
#define CRM_INT_SOFT2		0x40000000
#define CRM_CPU_SysCorErr	CRM_INT_SOFT2
#define CRM_INT_SOFT1		0x20000000
#define CRM_INT_SOFT0		0x10000000
#define CRM_INT_RE5		0x08000000
#define CRM_INT_RE4		0x04000000
#define CRM_INT_RE3		0x02000000
#define CRM_INT_RE2		0x01000000
#define CRM_INT_RE1		0x00800000
#define CRM_INT_RE0		0x00400000
#define CRM_INT_MEMERR		0x00200000
#define CRM_INT_CRMERR		0x00100000
#define CRM_INT_GBE3		0x00080000
#define CRM_INT_GBE2		0x00040000
#define CRM_INT_GBE1		0x00020000
#define CRM_INT_GBE0		0x00010000
#define MACE_PCI_SHARED2	0x00008000
#define MACE_PCI_SHARED1	0x00004000
#define MACE_PCI_SHARED0	0x00002000
#define MACE_PCI_SLOT2		0x00001000
#define MACE_PCI_SLOT1		0x00000800
#define MACE_PCI_SLOT0		0x00000400
#define MACE_PCI_SCSI1		0x00000200
#define MACE_PCI_SCSI0		0x00000100
#define MACE_PCI_BRIDGE		0x00000080
#define MACE_PERIPH_AUD		0x00000040
#define MACE_PERIPH_MISC	0x00000020
#define MACE_PERIPH_SERIAL	0x00000010
#define MACE_ETHERNET		0x00000008
#define MACE_VID_OUT		0x00000004
#define MACE_VID_IN2		0x00000002
#define MACE_VID_IN1		0x00000001
