/* $NetBSD: extern.h,v 1.2 2003/06/24 12:27:04 igy Exp $ */

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Naoto Shimazaki of YOKOGAWA Electric Corporation.
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

#ifndef _LOCORE
#include <sys/types.h>
#include <mips/cpuregs.h>

#include <dev/ic/comreg.h>
#include <dev/ic/ns16550reg.h>
#include <dev/ic/st16650reg.h>
#define com_lcr com_cfcr

struct bootmenu_command {
	const char	*c_name;
	void (*c_fn)(char*);
};

#define ROMCS0_BASE	0xbe000000U
#define ROMCS3_BASE	0xbf800000U
#define FLASH_BASE	ROMCS0_BASE

#ifdef ROMICE
#define KERN_ROMBASE	0x80800000U
#else
#define KERN_ROMBASE	0xbf800000U
#endif

#define __REG_1(reg)	*((volatile u_int8_t*) (reg))
#define __REG_2(reg)	*((volatile u_int16_t*) (reg))
#define __REG_4(reg)	*((volatile u_int32_t*) (reg))

#define ISSET(t, f)	((t) & (f))

#define REGWRITE_1(base, off, val)	\
		(__REG_1(MIPS_PHYS_TO_KSEG1((u_int32_t) (base) + (off))) \
		 = (val))
#define REGWRITE_2(base, off, val)	\
		(__REG_2(MIPS_PHYS_TO_KSEG1((u_int32_t) (base) + (off))) \
		 = (val))
#define REGWRITE_4(base, off, val)	\
		(__REG_4(MIPS_PHYS_TO_KSEG1((u_int32_t) (base) + (off))) \
		 = (val))

#define REGREAD_1(base, off)	\
		(__REG_1(MIPS_PHYS_TO_KSEG1((u_int32_t) (base) + (off))))
#define REGREAD_2(base, off)	\
		(__REG_2(MIPS_PHYS_TO_KSEG1((u_int32_t) (base) + (off))))
#define REGREAD_4(base, off)	\
		(__REG_4(MIPS_PHYS_TO_KSEG1((u_int32_t) (base) + (off))))

#define ISKEY	ISSET(REGREAD_1(VR4181_SIU_ADDR, com_lsr), LSR_RXRDY)

#define bus_space_write_1(iot, ioh, off, val)	REGWRITE_1((ioh), (off), (val))
#define bus_space_write_2(iot, ioh, off, val)	REGWRITE_2((ioh), (off), (val))
#define bus_space_write_4(iot, ioh, off, val)	REGWRITE_4((ioh), (off), (val))
#define bus_space_read_1(iot, ioh, off)	REGREAD_1((ioh), (off))
#define bus_space_read_2(iot, ioh, off)	REGREAD_2((ioh), (off))
#define bus_space_read_4(iot, ioh, off)	REGREAD_4((ioh), (off))
typedef void		*bus_space_tag_t;
typedef u_int32_t	bus_space_handle_t;
typedef size_t		bus_size_t;

void comcninit(void);
int iskey(void);
void start_netbsd(void);
int i28f128_probe(void *base);
int i28f128_region_write(void *dst, const void *src, size_t len);

/* dev_flash */
int flash_strategy(void *, int, daddr_t, size_t, void *, size_t *);
int flash_open(struct open_file *, ...);
int flash_close(struct open_file *);
int flash_ioctl(struct open_file *, u_long, void *);

#endif /* !_LOCORE */

#define	LCBOOT_STARTADDR	0x80001000
#define	LCBOOT_ROMSTARTADDR	0xbfd01000
#define	NETBSD_STARTADDR	0x80040000
