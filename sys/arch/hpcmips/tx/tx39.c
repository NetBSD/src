/*	$NetBSD: tx39.c,v 1.8.2.2 1999/12/27 18:32:10 wrstuden Exp $ */

/*
 * Copyright (c) 1999, by UCHIYAMA Yasushi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include "opt_tx39_debug.h"
#include "m38813c.h"
#include "p7416buf.h"
#include "tc5165buf.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/locore.h>   /* cpu_id */
#include <machine/bootinfo.h> /* bootinfo */
#include <machine/sysconf.h>  /* platform */

#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <hpcmips/hpcmips/machdep.h> /* cpu_model */

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
#if NP7416BUF > 0
#include <hpcmips/dev/p7416bufvar.h>
#endif
#if NM38813C > 0
#include <hpcmips/dev/m38813cvar.h>
#endif
#if NTC5165BUF > 0
#include <hpcmips/dev/tc5165bufvar.h>
#endif

extern unsigned nullclkread __P((void));
extern unsigned (*clkread) __P((void));

struct tx_chipset_tag tx_chipset;

#ifdef TX39_DEBUG
u_int32_t tx39debugflag;
#endif

void	tx_init __P((void));
int	tx39icu_intr __P((u_int32_t, u_int32_t, u_int32_t, u_int32_t));
int	tx39_find_dram __P((u_int32_t, u_int32_t));

/* TX39-specific initialization vector */
void	tx_os_init __P((void));
void	tx_bus_reset __P((void));
void	tx_cons_init __P((void));
void	tx_device_register __P((struct device *, void *));
void    tx_fb_init __P((caddr_t*));
int     tx_mem_init __P((caddr_t));
void	tx_reboot __P((int howto, char *bootstr));
int	tx_intr __P((u_int32_t mask, u_int32_t pc, u_int32_t statusReg, 
		     u_int32_t causeReg));

void
tx_init()
{
	tx_chipset_tag_t tc;
	int model, rev;
	
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

	model = (cpu_id.cpu.cp_majrev << 4)| cpu_id.cpu.cp_minrev;

	switch (model) {
	default:
		 /* Unknown TOSHIBA TX39-series */
		sprintf(cpu_model, "Unknown TOSHIBA TX39-series %x.%x", 
			cpu_id.cpu.cp_majrev, cpu_id.cpu.cp_minrev);
		break;
	case TMPR3912:
		sprintf(cpu_model, "TOSHIBA TMPR3912");
		cpuspeed = 50; /* XXX Should calibrate XXX */
		break;
	case TMPR3922:
		rev = tx_conf_read(tc, TX3922_REVISION_REG);
		sprintf(cpu_model, "TOSHIBA TMPR3922 rev. %x.%x",
			(rev >> 4) & 0xf, rev & 0xf);
		cpuspeed = 100; /* XXX Should calibrate XXX */
		break;
	}
}

void
tx_os_init()
{
	/*
	 * Set up interrupt handling and I/O addresses.
	 */
	mips_hardware_intr = tx39icu_intr;

	splvec.splbio = MIPS_SPL_2_4;
	splvec.splnet = MIPS_SPL_2_4;
	splvec.spltty = MIPS_SPL_2_4;
	splvec.splimp = MIPS_SPL_2_4;
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
	tx_chipset_tag_t tc;
	u_int32_t fb_start, fb_addr, fb_size, fb_line_bytes;

	/* Initialize to access TX39 configuration register */
	tc = tx_conf_get_tag();

	fb_start = MIPS_KSEG0_TO_PHYS(*kernend);
	tx3912video_init(tc, fb_start, bootinfo->fb_width,
			bootinfo->fb_height, &fb_addr, &fb_size, 
			&fb_line_bytes);

	/* Setup bootinfo */
	bootinfo->fb_line_bytes = fb_line_bytes;
	bootinfo->fb_addr = (unsigned char*)MIPS_PHYS_TO_KSEG1(fb_addr);

	/* Skip V-RAM area */
	*kernend += fb_size;
#endif /* TX391X */
#ifdef TX392X 
	/* 
	 *  Plum V-RAM isn't accessible until pmap_bootstrap,
	 * at this time, frame buffer device is disabled.
	 */
	bootinfo->fb_addr = 0;
#endif /* TX392X */
}

int
tx_mem_init(kernend)
	caddr_t kernend; /* kseg0 */
{
	u_int32_t startaddr, endaddr;
	int npage, xpage, kpage;

	startaddr = MIPS_PHYS_TO_KSEG1(
		(btoc((u_int32_t)kernend - MIPS_KSEG0_START)) << PGSHIFT);
	endaddr = MIPS_PHYS_TO_KSEG1(TX39_SYSADDR_DRAMBANK0CS1 +
				     TX39_SYSADDR_DRAMBANK_LEN);
	kpage = btoc(MIPS_KSEG1_TO_PHYS(startaddr));

	/* D-RAM bank0 */
	npage = tx39_find_dram(startaddr, endaddr);

	printf("DRAM bank0: %d pages (%dMByte) reserved %d pages\n", 
	       npage + 1, ((npage  + 1) * NBPG) / 0x100000, kpage + 1);
	npage -= kpage; /* exclude kernel area */

	/* Clear DRAM area */
	memset((void*)startaddr, 0, npage * NBPG);
	
	/* D-RAM bank1 XXX find only. not usable yet */
	startaddr = MIPS_PHYS_TO_KSEG1(TX39_SYSADDR_DRAMBANK1CS1);
	endaddr = MIPS_PHYS_TO_KSEG1(TX39_SYSADDR_DRAMBANK1CS1 +
				     TX39_SYSADDR_DRAMBANK_LEN);
	xpage = tx39_find_dram(startaddr, endaddr);
	printf("DRAM bank1: %d pages (%dMByte) ...but not usable yet\n", 
	       xpage + 1, ((xpage + 1) * NBPG) / 0x100000);

	/* 
	 *  Clear currently unused D-RAM area 
	 *  (For reboot Windows CE clearly)
	 */
	memset((void*)startaddr, 0, npage * NBPG);
	memset((void*)(KERNBASE + 0x400), 0, 
	       KERNTEXTOFF - KERNBASE - 0x800); 
	
	return npage; /* Return bank0's memory only */
}

void
tx_reboot(howto, bootstr)
	int howto;
	char *bootstr;
{
	goto *(u_int32_t *)MIPS_RESET_EXC_VEC;
}

int
tx39_find_dram(startaddr, endaddr)
	u_int32_t startaddr; /* kseg1 */
	u_int32_t endaddr;    /* kseg1 */
{
#define DRAM_MAGIC0 0xac1dcafe
#define DRAM_MAGIC1 0x19700220
	u_int32_t page;
	int npage;

	page = startaddr;
	((volatile int *)page)[0] = DRAM_MAGIC0;
	((volatile int *)page)[4] = DRAM_MAGIC1;
	page += NBPG;
	for (npage = 0; page < endaddr; page += NBPG, npage++) {
		if ((((volatile int *)page)[0] == DRAM_MAGIC0 &&
		     ((volatile int *)page)[4] == DRAM_MAGIC1)) {
			return npage;
		}
	}
	/* no memory in this bank */
	return 0;
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
#define CONSPLATIDMATCH(p) \
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
#if NP7416BUF > 0
		if(CONSPLATIDMATCH(COMPAQ_C) &&
		   p7416buf_cnattach(TX39_SYSADDR_CS3)) {
			panic("tx_cons_init: can't init console");
		}
#endif
#if NM38813C > 0
		if(CONSPLATIDMATCH(VICTOR_INTERLINK) &&
		   m38813c_cnattach(TX39_SYSADDR_CARD1)) {
			panic("tx_cons_init: can't init console");
		}
#endif
#if NTC5165BUF > 0
		if(CONSPLATIDMATCH(SHARP_TELIOS) &&
		   tc5165buf_cnattach(TX39_SYSADDR_CS1)) {
			panic("tx_cons_init: can't init console");
		}
#endif
	}

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
	if (tx_chipset.tc_intrt) {
		panic("duplicate intrt");
	}

	if (t != &tx_chipset) {
		panic("bogus tx_chipset_tag");
	}

	tx_chipset.tc_intrt = intrt;
}

#ifdef TX39_PREFER_FUNCTION
tx_chipset_tag_t
tx_conf_get_tag()
{
	return (tx_chipset_tag_t)&tx_chipset;
}

txreg_t
tx_conf_read(t, reg)
	tx_chipset_tag_t t;
	int reg;
{
	return *((txreg_t*)(TX39_SYSADDR_CONFIG_REG_KSEG1 + reg));
}

void
tx_conf_write(t, reg, val)
	tx_chipset_tag_t t;
	int reg;
	txreg_t val;
{
	*((txreg_t*)(TX39_SYSADDR_CONFIG_REG_KSEG1 + reg)) = val;
}
#endif /* TX39_PREFER_FUNCTION */

int
__is_set_print(reg, mask, name)
	u_int32_t reg;
	int mask;
	char *name;
{
	if (reg & mask) {
		printf("%s ", name);
		return 1;
	}
	return 0;
}
