/*	$NetBSD: cpu.h,v 1.2.4.2 2002/04/01 07:42:05 nathanw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Eduardo Horvath for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
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

#ifndef	_IBM4XX_CPU_H_
#define	_IBM4XX_CPU_H_

/* PVRs for different IBM CPUs */
#define	PVR_401A1		0x00210000
#define	PVR_401B2		0x00220000
#define	PVR_401C2		0x00230000
#define	PVR_401D2		0x00240000
#define	PVR_401E2		0x00250000
#define	PVR_401F2		0x00260000
#define	PVR_401G2		0x00270000

#define	PVR_403			0x00200000

#define PVR_405GP      		0x40110000 

#if defined(_KERNEL)
extern char bootpath[];

void calc_delayconst(void);
void ppc4xx_reset(void) __attribute__((__noreturn__));
#endif /* _KERNEL */

#include <powerpc/cpu.h>

/*
 * Board configuration structure from the OpenBIOS.
 */
struct board_cfg_data {
	unsigned char	usr_config_ver[4];
	unsigned char	rom_sw_ver[30];
	unsigned int	mem_size;
	unsigned char	mac_address_local[6];
	unsigned char	mac_address_pci[6];
	unsigned int	processor_speed;
	unsigned int	plb_speed;
	unsigned int	pci_speed;
} board_data;

extern struct board_cfg_data board_data;

/* Board info database stuff */
extern struct propdb *board_info;

#define	board_info_set(n, v, l, f, w)	\
	prop_set(board_info, 0, (n), (v), (l), (f), (w))
#define	board_info_get(n, v, l)		\
	prop_get(board_info, 0, (n), (v), (l), NULL)

#endif	/* _IBM4XX_CPU_H_ */
