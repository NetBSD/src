/*	$NetBSD: cpu.h,v 1.12 2006/05/05 18:04:42 thorpej Exp $	*/

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
#define PVR_405GP_PASS1 	0x40110000	/* RevA */ 
#define PVR_405GP_PASS2 	0x40110040	/* RevB */ 
#define PVR_405GP_PASS2_1 	0x40110082	/* RevC */ 
#define PVR_405GP_PASS3 	0x401100c4	/* RevD */ 
#define PVR_405GPR     		0x50910000
#define PVR_405GPR_REVB		0x50910951

#if defined(_KERNEL)
extern char bootpath[];

#include <sys/param.h>
#include <sys/device.h>
#include <prop/proplib.h>

/* export from ibm4xx/autoconf.c */
extern void (*md_device_register) __P((struct device *dev, void *aux));

/* export from ibm4xx/machdep.c */
extern void (*md_consinit) __P((void));
extern void (*md_cpu_startup) __P((void));

/* export from ibm4xx/ibm40x_machdep.c */
extern void ibm40x_memsize_init(u_int, u_int);

/* export from ibm4xx/ibm4xx_machdep.c */
extern void ibm4xx_init(void (*)(void));
extern void ibm4xx_cpu_startup(const char *);
extern void ibm4xx_dumpsys(void);
extern void ibm4xx_install_extint(void (*)(void));

/* export from ibm4xx/ibm4xx_autoconf.c */
extern void ibm4xx_device_register(struct device *dev, void *aux);

/* export from ibm4xx/clock.c */
extern void calc_delayconst(void);

/* export from ibm4xx/4xx_locore.S */
extern void ppc4xx_reset(void) __attribute__((__noreturn__));

#endif /* _KERNEL */

#include <powerpc/cpu.h>

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
