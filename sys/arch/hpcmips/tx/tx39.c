/*	$NetBSD: tx39.c,v 1.39.18.1 2010/02/01 04:18:31 matt Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tx39.c,v 1.39.18.1 2010/02/01 04:18:31 matt Exp $");

#include "opt_vr41xx.h"
#include "opt_tx39xx.h"
#include "m38813c.h"
#include "tc5165buf.h"

#include <sys/param.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <mips/cache.h>

#include <machine/bootinfo.h> /* bootinfo */
#include <machine/sysconf.h>  /* platform */

#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <machine/bus.h>

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

struct tx_chipset_tag tx_chipset;

void	tx_init(void);
#if defined(VR41XX) && defined(TX39XX)
#define	TX_INTR	tx_intr
#else
#define	TX_INTR	cpu_intr	/* locore_mips3 directly call this */
#endif

extern void TX_INTR(u_int32_t, u_int32_t, u_int32_t, u_int32_t);

void	tx39clock_cpuspeed(int *, int *);

/* TX39-specific initialization vector */
void	tx_cons_init(void);
void    tx_fb_init(void **);
void    tx_mem_init(paddr_t);
void	tx_find_dram(paddr_t, paddr_t);
void	tx_reboot(int, char *);

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
	platform.cpu_intr	= TX_INTR;
	platform.cpu_idle	= NULL; /* not implemented yet */
	platform.cons_init	= tx_cons_init;
	platform.fb_init	= tx_fb_init;
	platform.mem_init	= tx_mem_init;
	platform.reboot		= tx_reboot;


	model = MIPS_PRID_REV(mips_options.mips_cpu_id);

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
tx_fb_init(void **kernend)
{
#ifdef TX391X
	paddr_t fb_end;

	fb_end = MIPS_KSEG0_TO_PHYS(mem_clusters[0].start + 
	    mem_clusters[0].size - 1);
	tx3912video_init(MIPS_KSEG0_TO_PHYS(*kernend), &fb_end);
			 
	/* Skip V-RAM area */
	*kernend = (void *)MIPS_PHYS_TO_KSEG0(fb_end);
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
tx_mem_init(paddr_t kernend)
{

	mem_clusters[0].start = 0;
	mem_clusters[0].size = kernend;
	mem_cluster_cnt = 1;
	/* search DRAM bank 0 */
	tx_find_dram(kernend, 0x02000000);
	
	/* search DRAM bank 1 */
	tx_find_dram(0x02000000, 0x04000000);
}

void
tx_find_dram(paddr_t start, paddr_t end)
{
	char *page, *startaddr, *endaddr;
	u_int32_t magic0, magic1;
#define MAGIC0		(*(volatile u_int32_t *)(page + 0))
#define MAGIC1		(*(volatile u_int32_t *)(page + 4))

	startaddr = (char *)MIPS_PHYS_TO_KSEG1(start);
	endaddr = (char *)MIPS_PHYS_TO_KSEG1(end);

	page = startaddr;
	if (badaddr(page, 4))
		return;

	do {
		magic0 = random();
		magic1 = random();
	} while (MAGIC0 == magic0 || MAGIC0 == magic1);

	MAGIC0 = magic0;
	MAGIC1 = magic1;
	wbflush();

	if (MAGIC0 != magic0 || MAGIC1 != magic1)
		return;

	for (page += PAGE_SIZE; page < endaddr; page += PAGE_SIZE) {
		if (badaddr(page, 4))
			return;
		if (MAGIC0 == magic0 &&
		    MAGIC1 == magic1) {
			goto memend_found;
		}
	}

	/* check for 32MByte memory */
	page -= PAGE_SIZE;
	MAGIC0 = magic0;
	MAGIC1 = magic1;
	wbflush();
	if (MAGIC0 != magic0 || MAGIC1 != magic1)
		return; /* no memory in this bank */

 memend_found:
	mem_clusters[mem_cluster_cnt].start = start;
	mem_clusters[mem_cluster_cnt].size = page - startaddr;

	/* skip kernel area */
	if (mem_cluster_cnt == 1)
		mem_clusters[mem_cluster_cnt].size -= start;

	mem_cluster_cnt++;
#undef MAGIC0
#undef MAGIC1
}

void
tx_reboot(int howto, char *bootstr)
{

	goto *(u_int32_t *)MIPS_RESET_EXC_VEC;
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
		    (TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8)) {
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
tx_conf_register_intr(tx_chipset_tag_t t, void *intrt)
{

	KASSERT(t == &tx_chipset);
	tx_chipset.tc_intrt = intrt;
}

void
tx_conf_register_power(tx_chipset_tag_t t, void *powert)
{

	KASSERT(t == &tx_chipset);
	tx_chipset.tc_powert = powert;
}

void
tx_conf_register_clock(tx_chipset_tag_t t, void *clockt)
{

	KASSERT(t == &tx_chipset);
	tx_chipset.tc_clockt = clockt;
}

void
tx_conf_register_sound(tx_chipset_tag_t t, void *soundt)
{

	KASSERT(t == &tx_chipset);
	tx_chipset.tc_soundt = soundt;
}

void
tx_conf_register_video(tx_chipset_tag_t t, void *videot)
{

	KASSERT(t == &tx_chipset);
	tx_chipset.tc_videot = videot;
}
