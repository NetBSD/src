/*	$NetBSD: idprom.h,v 1.1 2001/04/06 13:13:03 fredette Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef	_MACHINE_IDPROM_H
#define	_MACHINE_IDPROM_H

/*
 * structure/definitions for the 32 byte id prom found in all suns.
 */

struct idprom {
    unsigned char idp_format;
    unsigned char idp_machtype;
    unsigned char idp_etheraddr[6];
    long          idp_date;
    unsigned char idp_serialnum[3];
    unsigned char idp_checksum;
    /* Note: The rest is excluded from the checksum! */
    unsigned char idp_reserved[16];
};

#define IDPROM_VERSION 1
#define IDPROM_SIZE (sizeof(struct idprom))
#define IDPROM_CKSUM_SIZE 16

/* values for cpu_machine_id */

/* High nibble identifies the architecture. */
#define IDM_ARCH_MASK	0xf0
#define IDM_ARCH_SUN2	0x00
#define IDM_ARCH_SUN3	0x10
#define IDM_ARCH_SUN4   0x20
#define IDM_ARCH_SUN3X	0x40
#define IDM_ARCH_SUN4C	0x50
#define IDM_ARCH_SUN4M	0x70

/* Low nibble identifies the implementation. */
#define IDM_IMPL_MASK 0x0f

/* Values of idp_machtype we might see (m68k only). */
#define SUN2_MACH_120  0x01	/* Sun2 Multibus */
#define SUN2_MACH_50   0x02	/* Sun2 VME */
#define SUN3_MACH_160  0x11 	/* Carrera */
#define SUN3_MACH_50   0x12 	/* M25 */
#define SUN3_MACH_260  0x13 	/* Sirius */
#define SUN3_MACH_110  0x14 	/* Prism */
#define SUN3_MACH_60   0x17 	/* Sun3F */
#define SUN3_MACH_E    0x18 	/* Sun3E */

#define SUN3X_MACH_470  0x41	/* Pegasus */
#define SUN3X_MACH_80   0x42	/* Hydra */

#if defined(_KERNEL) || defined(_STANDALONE)

extern struct idprom identity_prom;
extern u_char cpu_machine_id;

void idprom_etheraddr __P((u_char *));
void idprom_init __P((void));

#endif	/* _KERNEL || _STANDALONE */

#endif /* !_MACHINE_IDPROM_H */
