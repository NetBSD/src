/*	$NetBSD: sbd.h,v 1.1 2005/12/29 15:20:09 tsutsui Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

#ifndef _EWS4800MIPS_SBD_H_
#define	_EWS4800MIPS_SBD_H_
/* System board model independ definition */

struct sbdinfo {
	uint32_t machine	:16;
	uint32_t model		:16;
	uint32_t mmu		:1;
	uint32_t cache		:1;
	uint32_t panel		:2;
	uint32_t padding1	:12;
	uint32_t fdd		:8;
	uint32_t kb		:8;
	uint32_t cpu		:4;
	uint32_t fpp		:4;
	uint32_t fpa		:4;
	uint32_t iop		:4;
	uint32_t padding2	:16;	/* ----350---- */
	uint32_t clock		:32;
	char	 model_name[20];
	uint32_t padding3	:24;
	uint32_t ipl		:8;
	uint32_t cpu_ex		:32;
	uint32_t fpp_ex		:32;	/* ----360---- */
	uint32_t padding4	:16;
	uint32_t kbms		:4;
	uint32_t sio		:4;
	uint32_t battery	:8;
	uint32_t scsi		:32;
} __attribute__((__packed__));

enum sbd_machine_type {
	MACHINE_TR2	= 0x1017,	/* EWS4800/350 */
	MACHINE_TR2A	= 0x101f	/* EWS4800/360 */
};

/* Model independent ROM entries */
#define	SBD_INFO	((struct sbdinfo *)0xbfc0fe00)
#define	ROM_ADDR	0xbfc00000
#define	ROM_SIZE	0x00080000
/* ROM_DK_RW(unit|flag, sector, count, addr) */
#define	ROM_DK_RW	((int (*)(int, uint32_t, uint32_t, void *))0xbfc0ff30)
#define	ROM_DK_READ(u, s, c, a)		ROM_DK_RW(u, s, c, a)
#define	ROM_DK_WRITE(u, s, c, a)	ROM_DK_RW(u | 0x1000, s, c, a)
/* int ROM_GETC(void) */
#define	ROM_GETC	((int (*)(void))0xbfc0ff50)
/* void ROM_PUTC(xpixel, ypixel, c) */
#define	ROM_PUTC	((void (*)(int, int, int))0xbfc0ff60)
/* ROM_FD_RW(unit | flag, cylinder << 16 | side << 8 | sector, count, addr) */
#define	ROM_FD_RW	((int (*)(int, uint32_t, uint32_t, void *))0xbfc0ff20)
#define	ROM_FD_READ(u, s, c, a)		ROM_FD_RW(u, s, c, a)
#define	ROM_FD_WRITE(u, s, c, a)	ROM_FD_RW(u | 0x1000, s, c, a)

#define	ROM_ETHER_IPL	0xbfc0ff40
#define	ROM_CGMT_READ	0xbfc0ff48
#define	ROM_PRINTF	0xbfc0ff58
#define	ROM_NETIPL	0xbfc0ff68

#define	ROM_MONITOR	((void (*)(void))0xbfc0ff08)
#define	ROM_CPUITF	0xbfc0ff00	/* machine check */
#define	ROM_SPP		0xbfc0ff70
#define	ROM_SPPBP	0xbfc0ff78

#define	ROM_FONT_WIDTH		12
#define	ROM_FONT_HEIGHT		24

/* NVSRAM */
#define	NVSRAM_BOOTDEV_MIN		0
#define	NVSRAM_BOOTDEV_FLOPPYDISK	0
#define	NVSRAM_BOOTDEV_HARDDISK		2
#define	NVSRAM_BOOTDEV_CGMT		4
#define	NVSRAM_BOOTDEV_NETWORK		6
#define	NVSRAM_BOOTDEV_NETWORK_T_AND_D	8
#define	NVSRAM_BOOTDEV_MAX		8

/* RAM */
struct mainfo_type1 {
	uint32_t reserved:14,
	    m8:2, m7:2, m6:2, m5:2, m4:2, m3:2, m2:2, m1:2, m0:2;
};
/* TR2, TR2A */
struct mainfo_type2 {
	uint32_t m7:4, m6:4, m5:4, m4:4, m3:4, m2:4, m1:4, m0:4;
};
#define	MA0_ADDR		0x00000000
#define	__M0_BANK0_ADDR		0x00000000
#define	__M0_BANK1_ADDR		0x04000000
#define	MA1_ADDR		0x08000000
#define	__M1_BANK0_ADDR		0x08000000
#define	__M1_BANK1_ADDR		0x0c000000
#define	MA2_ADDR		0x10000000
#define	__M2_BANK0_ADDR		0x10000000
#define	__M2_BANK1_ADDR		0x14000000

#define	TR2A_MA3_ADDR		0x20000000
#define	TR2_MA3_ADDR		0x38000000

#endif /* !_EWS4800MIPS_SBD_H_ */
