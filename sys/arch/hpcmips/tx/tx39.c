/*	$NetBSD: tx39.c,v 1.21 2001/04/12 19:22:52 thorpej Exp $ */

/*-
 * Copyright (c) 1999, 2000 The NetBSD Foundation, Inc.
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

#include "opt_tx39_debug.h"
#include "m38813c.h"
#include "tc5165buf.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kcore.h>

#include <machine/locore.h>   /* cpu_id */
#include <machine/bootinfo.h> /* bootinfo */
#include <machine/sysconf.h>  /* platform */

#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <hpcmips/hpcmips/machdep.h> /* cpu_name */

#include <hpcmips/tx/tx39biureg.h>
#include <hpcmips/tx/tx39reg.h>
#include <hpcmips/tx/tx39var.h>
#ifdef TX391X
#include <hpcmips/tx/tx3912videovar.h>
#endif

#include <sys/termios.h>
#include <sys/ttydefaults.h>
#include <hpcmips/tx/tx39uartvar.h>
#ifndef CONSPEED
#define CONSPEED TTYDEF_SPEED
#endif

/* console keyboard */
#if NM38813C > 0
#include <hpcmips/dev/m38813cvar.h>
#endif
#if NTC5165BUF > 0
#include <hpcmips/dev/tc5165bufvar.h>
#endif

extern unsigned nullclkread(void);
extern unsigned (*clkread)(void);

struct tx_chipset_tag tx_chipset;

#ifdef TX39_DEBUG
u_int32_t tx39debugflag;
#endif

void	tx_init(void);
int	tx39icu_intr(u_int32_t, u_int32_t, u_int32_t, u_int32_t);
void	tx39clock_cpuspeed(int*, int*);

/* TX39-specific initialization vector */
void	tx_os_init(void);
void	tx_bus_reset(void);
void	tx_cons_init(void);
void	tx_device_register(struct device *, void *);
void    tx_fb_init(caddr_t*);
void    tx_mem_init(paddr_t);
void	tx_find_dram(paddr_t, paddr_t);
void	tx_reboot(int, char *);
int	tx_intr(u_int32_t, u_int32_t, u_int32_t, u_int32_t);

extern phys_ram_seg_t mem_clusters[];
extern int mem_cluster_cnt;

void
tx_init()
{
	tx_chipset_tag_t tc;
	int model, rev;
	int cpuclock;
	
	tc = tx_conf_get_tag();
	/*
	 * Platform Specific Function Hooks
	 */
	platform.os_init = tx_os_init;
	platform.bus_reset = tx_bus_reset;
	platform.cons_init = tx_cons_init;
	platform.device_register = tx_device_register;
	platform.fb_init = tx_fb_init;
	platform.mem_init = tx_mem_init;
	platform.reboot = tx_reboot;
	platform.iointr = tx39icu_intr;

	model = MIPS_PRID_REV(cpu_id);

	switch (model) {
	default:
		 /* Unknown TOSHIBA TX39-series */
		sprintf(cpu_name, "Unknown TOSHIBA TX39-series %x", model);
		break;
	case TMPR3912:
		tx39clock_cpuspeed(&cpuclock, &cpuspeed);

		sprintf(cpu_name, "TOSHIBA TMPR3912 %d.%02d MHz",
			cpuclock / 1000000, (cpuclock % 1000000) / 10000);
		tc->tc_chipset = __TX391X;
		break;
	case TMPR3922:
		tx39clock_cpuspeed(&cpuclock, &cpuspeed);
		rev = tx_conf_read(tc, TX3922_REVISION_REG);

		sprintf(cpu_name, "TOSHIBA TMPR3922 rev. %x.%x "
			"%d.%02d MHz", (rev >> 4) & 0xf, rev & 0xf, 
			cpuclock / 1000000, (cpuclock % 1000000) / 10000);
		tc->tc_chipset = __TX392X;
		break;
	}
}

void
tx_os_init()
{
	/*
	 * Set up interrupt handling and I/O addresses.
	 */

	splvec.splbio = MIPS_SPL_2_4;
	splvec.splnet = MIPS_SPL_2_4;
	splvec.spltty = MIPS_SPL_2_4;
	splvec.splvm = MIPS_SPL_2_4;
	splvec.splclock = MIPS_SPL_2_4;
	splvec.splstatclock = MIPS_SPL_2_4;
	
	/* no high resolution timer circuit; possibly never called */
	clkread = nullclkread;
}

void
tx_fb_init(kernend)
	caddr_t *kernend;
{
#ifdef TX391X
	paddr_t fb_end;

	fb_end = MIPS_KSEG0_TO_PHYS(mem_clusters[0].start + 
				    mem_clusters[0].size - 1);
	tx3912video_init(MIPS_KSEG0_TO_PHYS(*kernend), &fb_end);
			 
	/* Skip V-RAM area */
	*kernend = (caddr_t)MIPS_PHYS_TO_KSEG0(fb_end);
#endif /* TX391X */
#ifdef TX392X 
	/* 
	 *  Plum V-RAM isn't accessible until pmap_bootstrap,
	 * at this time, frame buffer device is disabled.
	 */
	bootinfo->fb_addr = 0;
#endif /* TX392X */
}

void
tx_mem_init(kernend)
	paddr_t kernend;
{
	mem_clusters[0].start = 0;
	mem_clusters[0].size = kernend;
	mem_cluster_cnt = 1;
	/* search DRAM bank 0 */
	tx_find_dram(kernend, 0x02000000);
	
	/* search DRAM bank 1 */
	tx_find_dram(0x02000000, 0x04000000);
	/* 
	 *  Clear currently unused D-RAM area 
	 *  (For reboot Windows CE clearly)
	 */
	memset((void *)(KERNBASE + 0x400), 0, KERNTEXTOFF - 
	       (KERNBASE + 0x800));
}

void
tx_find_dram(start, end)
	paddr_t start, end;
{
	caddr_t page, startaddr, endaddr;

	startaddr = (void*)MIPS_PHYS_TO_KSEG1(start);
	endaddr = (void*)MIPS_PHYS_TO_KSEG1(end);

#define DRAM_MAGIC0 0xac1dcafe
#define DRAM_MAGIC1 0x19700220

	page = startaddr;
	if (badaddr(page, 4))
		return;

	*(volatile int *)(page + 0) = DRAM_MAGIC0;
	*(volatile int *)(page + 4) = DRAM_MAGIC1;
	wbflush();

	if (*(volatile int *)(page + 0) != DRAM_MAGIC0 ||
	    *(volatile int *)(page + 4) != DRAM_MAGIC1)
		return;

	for (page += NBPG; page < endaddr; page += NBPG) {
		if (badaddr(page, 4))
			return;

		if (*(volatile int *)(page + 0) == DRAM_MAGIC0 &&
		    *(volatile int *)(page + 4) == DRAM_MAGIC1) {
			goto memend_found;
		}
	}

	/* check for 32MByte memory */
	page -= NBPG;
	*(volatile int *)(page + 0) = DRAM_MAGIC0;
	*(volatile int *)(page + 4) = DRAM_MAGIC1;
	wbflush();

	if (*(volatile int *)(page + 0) != DRAM_MAGIC0 ||
	    *(volatile int *)(page + 4) != DRAM_MAGIC1)
		return; /* no memory in this bank */

 memend_found:
	mem_clusters[mem_cluster_cnt].start = start;
	mem_clusters[mem_cluster_cnt].size = page - startaddr;

	/* skip kernel area */
	if (mem_cluster_cnt == 1)
		mem_clusters[mem_cluster_cnt].size -= start;
	
	mem_cluster_cnt++;
}

void
tx_reboot(howto, bootstr)
	int howto;
	char *bootstr;
{
	goto *(u_int32_t *)MIPS_RESET_EXC_VEC;
}

void
tx_bus_reset()
{
	/* hpcmips port don't use */
}

void
tx_cons_init()
{
	int slot;
#define CONSPLATIDMATCH(p)						\
	platid_match(&platid, &platid_mask_MACH_##p)

#ifdef SERIALCONSSLOT
	slot = SERIALCONSSLOT;
#else
	slot = TX39_UARTA;
#endif
	if (bootinfo->bi_cnuse & BI_CNUSE_SERIAL) {
		if(txcom_cnattach(slot, CONSPEED,
				  (TTYDEF_CFLAG & ~(CSIZE | PARENB)) | 
				  CS8)) {
			panic("tx_cons_init: can't attach serial console.");
		}
	} else {
#if NM38813C > 0
		if(CONSPLATIDMATCH(VICTOR_INTERLINK) &&
		   m38813c_cnattach(TX39_SYSADDR_CARD1)) {
			goto panic;
		}
#endif
#if NTC5165BUF > 0
		if(CONSPLATIDMATCH(COMPAQ_C) &&
		   tc5165buf_cnattach(TX39_SYSADDR_CS3)) {
			goto panic;
		}

		if(CONSPLATIDMATCH(SHARP_TELIOS) &&
		   tc5165buf_cnattach(TX39_SYSADDR_CS1)) {
			goto panic;
		}
		
		if(CONSPLATIDMATCH(SHARP_MOBILON) &&
		   tc5165buf_cnattach(TX39_SYSADDR_MCS0)) {
			goto panic;
		}
#endif
	}
	
	return;
 panic:
	panic("tx_cons_init: can't init console");
	/* NOTREACHED */
}

void
tx_device_register(dev, aux)
	struct device *dev;
	void *aux;
{
	/* hpcmips port don't use */
}

void
tx_conf_register_intr(t, intrt)
	tx_chipset_tag_t t;
	void *intrt;
{
	KASSERT(t == &tx_chipset);

	tx_chipset.tc_intrt = intrt;
}

void
tx_conf_register_power(t, powert)
	tx_chipset_tag_t t;
	void *powert;
{
	KASSERT(t == &tx_chipset);

	tx_chipset.tc_powert = powert;
}

void
tx_conf_register_clock(t, clockt)
	tx_chipset_tag_t t;
	void *clockt;
{
	KASSERT(t == &tx_chipset);

	tx_chipset.tc_clockt = clockt;
}

void
tx_conf_register_sound(t, soundt)
	tx_chipset_tag_t t;
	void *soundt;
{
	KASSERT(t == &tx_chipset);

	tx_chipset.tc_soundt = soundt;
}

void
tx_conf_register_ioman(tx_chipset_tag_t t, struct txio_ops *ops)
{
	KASSERT(t == &tx_chipset);
	KASSERT(ops);

	tx_chipset.tc_ioops[ops->_group] = ops;
}

void
tx_conf_register_video(t, videot)
	tx_chipset_tag_t t;
	void *videot;
{
	KASSERT(t == &tx_chipset);

	tx_chipset.tc_videot = videot;
}

int
__is_set_print(reg, mask, name)
	u_int32_t reg;
	int mask;
	char *name;
{
	const char onoff[2] = "_x";
	int ret = reg & mask ? 1 : 0;

	printf("%s[%c] ", name, onoff[ret]);

	return ret;
}
