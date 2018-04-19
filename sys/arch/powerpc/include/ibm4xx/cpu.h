/*	$NetBSD: cpu.h,v 1.22 2018/04/19 21:50:07 christos Exp $	*/

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

#include <powerpc/psl.h>
#include <powerpc/spr.h>
#include <powerpc/ibm4xx/spr.h>
#include <powerpc/ibm4xx/dcr4xx.h>

#if defined(_KERNEL)
extern char bootpath[];

struct exc_info {
	vaddr_t exc_vector;
	const uint32_t *exc_addr; 
	uintptr_t exc_size;
}; 

#include <sys/param.h>
#include <sys/device.h>
#include <prop/proplib.h>

/* export from ibm4xx/autoconf.c */
extern void (*md_device_register)(device_t dev, void *aux);

/* export from ibm4xx/machdep.c */
extern void (*md_consinit)(void);
extern void (*md_cpu_startup)(void);

/* export from ibm4xx/ibm40x_machdep.c */
extern void ibm40x_memsize_init(u_int, u_int);

/* export from ibm4xx/ibm4xx_machdep.c */
extern void ibm4xx_init(vaddr_t, vaddr_t, void (*)(void));
extern void ibm4xx_cpu_startup(const char *);
extern void ibm4xx_dumpsys(void);
extern void ibm4xx_install_extint(void (*)(void));

/* export from ibm4xx/ibm4xx_autoconf.c */
extern void ibm4xx_device_register(device_t dev, void *aux);

/* export from ibm4xx/clock.c */
extern void calc_delayconst(void);

/* export from ibm4xx/4xx_locore.S */
extern void ppc4xx_reset(void) __dead;

extern void intr_init(void);

/*
 * DCR (Device Control Register) access. These have to be
 * macros because register address is encoded as immediate
 * operand.
 */
static __inline void
mtdcr(int reg, uint32_t val)
{
	__asm volatile("mtdcr %0,%1" : : "K"(reg), "r"(val));
}

static __inline uint32_t
mfdcr(int reg)
{
	uint32_t val;	
	
	__asm volatile("mfdcr %0,%1" : "=r"(val) : "K"(reg));
	return val;
}

static __inline void
mtcpr(int reg, uint32_t val)
{
	mtdcr(DCR_CPR0_CFGADDR, reg);
	mtdcr(DCR_CPR0_CFGDATA, val);
}

static __inline uint32_t
mfcpr(int reg)
{
	mtdcr(DCR_CPR0_CFGADDR, reg);
	return mfdcr(DCR_CPR0_CFGDATA);
}

static void inline
mtsdr(int reg, uint32_t val)
{
	mtdcr(DCR_SDR0_CFGADDR, reg);
	mtdcr(DCR_SDR0_CFGDATA, val);
}

static __inline uint32_t
mfsdr(int reg)
{
	mtdcr(DCR_SDR0_CFGADDR, reg);
	return mfdcr(DCR_SDR0_CFGDATA);
}

#include <powerpc/pic/picvar.h>

extern struct pic_ops pic_uic403;
extern struct pic_ops pic_uic0;
extern struct pic_ops pic_uic1;
extern struct pic_ops pic_uic2;

extern paddr_t msgbuf_paddr;
extern vaddr_t msgbuf_vaddr;
extern char msgbuf[MSGBUFSIZE];
#endif /* _KERNEL */

/* Board info dictionary */
extern prop_dictionary_t board_properties;
extern void board_info_init(void);

/*****************************************************************************/
/* THIS CODE IS OBSOLETE. WILL BE REMOVED */
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
};

extern struct board_cfg_data board_data;
/*****************************************************************************/

#endif	/* _IBM4XX_CPU_H_ */
