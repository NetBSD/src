/*	$NetBSD: mca_machdep.c,v 1.2 2000/06/04 19:14:47 cgd Exp $	*/

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * Copyright (c) 1996-1999 Scott D. Telford. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
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
 * Machine-specific functions for MCA autoconfiguration.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/syslog.h>

#include <machine/bioscall.h>
#include <machine/psl.h>

#define _I386_BUS_DMA_PRIVATE
#include <machine/bus.h>

#include <i386/isa/icu.h>
#include <dev/isa/isavar.h>
#include <dev/isa/isareg.h>
#include <dev/mca/mcavar.h>
#include <dev/mca/mcareg.h>

#include "isa.h"
#include "opt_mcaverbose.h"

struct i386_bus_dma_tag mca_bus_dma_tag = {
	NULL,			/* _cookie */
	_bus_dmamap_create,
	_bus_dmamap_destroy,
	_bus_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	_bus_dmamap_unload,
	NULL,			/* _dmamap_sync */
	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap,
};

/* setup by mca_busprobe() */
int MCA_system = 1; /* XXX force MCA for now */
int bios_features = 1;

/* System Configuration Block */
struct sys_config {
	u_int16_t	count;
	u_int8_t	model;
	u_int8_t	submodel;
	u_int8_t	bios_rev;
	u_int8_t	feature;
#define FEATURE_RESV	0x01	/* Reserved				*/
#define FEATURE_MCABUS	0x02	/* MicroChannel Architecture		*/
#define FEATURE_EBDA	0x04	/* Extended BIOS data area allocated	*/
#define FEATURE_WAITEV	0x08	/* Wait for external event is supported	*/
#define FEATURE_KBDINT	0x10	/* Keyboard intercept called by Int 09h	*/
#define FEATURE_RTC	0x20	/* Real-time clock present		*/
#define FEATURE_IC2	0x40	/* Second interrupt chip present	*/
#define FEATURE_DMA3	0x80	/* DMA channel 3 used by hard disk BIOS	*/
	u_int8_t	pad[3];
} __attribute__ ((packed));

void
mca_attach_hook(parent, self, mba)
	struct device *parent, *self;
	struct mcabus_attach_args *mba;
{
	/* Nothing */
}

/*
 * Read value of MCA POS register "reg" in slot "slot".
 */

int
mca_conf_read(mc, slot, reg)
	mca_chipset_tag_t mc;
	int slot, reg;
{
	int	data;

	slot &= 7;	/* slot must be in range 0-7 */	
	outb(MCA_MB_SETUP_REG, 0xff); /* ensure m/board setup is disabled */
	outb(MCA_ADAP_SETUP_REG, slot | MCA_ADAP_SET);
	data = inb(MCA_POS_REG(reg));
	outb(MCA_ADAP_SETUP_REG, 0);
	return data;
}


/*
 * Write "data" to MCA POS register "reg" in slot "slot".
 */

void
mca_conf_write(mc, slot, reg, data)
	mca_chipset_tag_t mc;
	int slot, reg, data;
{
	slot&=7;	/* slot must be in range 0-7 */	
	outb(MCA_MB_SETUP_REG, 0xff); /* ensure m/board setup is disabled */
	outb(MCA_ADAP_SETUP_REG, slot | MCA_ADAP_SET);
	outb(MCA_POS_REG(reg), data);
	outb(MCA_ADAP_SETUP_REG, 0);
}

#if NISA <= 0
#error mca_intr_(dis)establish: needs ISA to be configured into kernel
#endif

const struct evcnt *
mca_intr_establish(mca_chipset_tag_t mc, mca_intr_handle_t ih)
{

	/* XXX for now, no evcnt parent reported */
	return NULL;
}

void *
mca_intr_establish(mc, ih, level, func, arg)
	mca_chipset_tag_t mc;
	mca_intr_handle_t ih;
	int level, (*func) __P((void *));
	void *arg;
{
	if (ih == 0 || ih >= ICU_LEN || ih == 2)
		panic("mca_intr_establish: bogus handle 0x%x\n", ih);

	/* MCA interrupts are always level-triggered */
	return isa_intr_establish(NULL, ih, IST_LEVEL, level, func, arg);
}

void
mca_intr_disestablish(mc, cookie)
	mca_chipset_tag_t mc;
	void *cookie;
{
	return isa_intr_disestablish(NULL, cookie);
}
	

/*
 * Handle a NMI.
 * return true to panic system, false to ignore.
 */
int
mca_nmi()
{
	/*
	* PS/2 MCA devices can generate NMIs - we can find out which
	* slot generated it from the POS registers.
	*/
 
	int 	slot, mcanmi=0;

	/* if there is no MCA bus, call isa_nmi() */
	if (!MCA_system)
		goto out;

	/* ensure motherboard setup is disabled */
	outb(MCA_MB_SETUP_REG, 0xff);

	/* find if an MCA slot has the CHCK bit asserted (low) in POS 5 */
	for(slot=0; slot<MCA_MAX_SLOTS; slot++)
	{
		outb(MCA_ADAP_SETUP_REG, slot | MCA_ADAP_SET);
		if ((inb(MCA_POS_REG(5)) & MCA_POS5_CHCK) == 0)
		{
			mcanmi = 1;
			/* find if CHCK status is available in POS 6/7 */
			if((inb(MCA_POS_REG(5)) & MCA_POS5_CHCK_STAT) == 0)
				log(LOG_CRIT, "MCA NMI: slot %d, POS6=0x%02x, POS7=0x%02x\n",
					slot+1, inb(MCA_POS_REG(6)),
						inb(MCA_POS_REG(7)));
			else
				log(LOG_CRIT, "MCA NMI: slot %d\n", slot+1);
		}
	}
	outb(MCA_ADAP_SETUP_REG, 0);

   out:
#if NISA > 0
	if (!mcanmi) {
		/* no CHCK bits asserted, assume ISA NMI */
		return (isa_nmi());
	} else
#endif
		return(0);
}

void
mca_busprobe()
{
	/* According to Linux's linux/arch/i386/boot/setup.S,
	 * we can get the extra BIOS information via int 0x15,
	 * ah == 0xc0. The extra information is stored on
	 * es:bx; it contains two bytes of length, then byte
	 * of machine id, byte of machine submodel, byte of BIOS
	 * revision number and byte of feature info.
	 */
	/*
	 * Scott Telford's code used 
	 * ((inb(MCA_ADAP_SETUP_REG) & (MCA_ADAP_SET | MCA_ADAP_CHR)) == 0)
	 * - somewhat dubious, but good enough.
	 */

#if notyet
	/*
	 * Following has been taken from FreeBSD, for now just for
	 * documentation purposes
	 */
	struct bioscallregs regs;
	struct sys_config *scp;
	paddr_t             paddr;

	memset(&regs, 0, sizeof(regs));
	regs.AX_HI = 0xc0;
	bioscall(0x15, &regs);

	if ((regs.EFLAGS & PSL_C)
	     || (regs.AX_HI != 0 && (regs.FLAGS & PSL_AC)))
	{
#ifdef MCAVERBOSE
		printf("BIOS SDT: Not supported. Not PS/2?\n");
#endif
		return;
	}
	
	paddr = (regs.ES << 4) + regs.BX;
	scp = (struct sys_config *)ISA_HOLE_VADDR(paddr);

	printf("BIOS SDT: model 0x%x, submodel 0x%x, BIOS rev. 0x%x\n",
		scp->model, scp->submodel, scp->bios_rev);

#ifdef MCAVERBOSE
	printf("BIOS SDT: features 0x%b\n", scp->feature,
			"\20"
			"\01RESV"
			"\02MCABUS"
			"\03EBDA"
			"\04WAITEV"
			"\05KBDINT"
			"\06RTC"
			"\07IC2"
			"\08DMA3\n");
#endif

	bios_features = scp->feature;
	MCA_system = (bios_features & FEATURE_MCABUS) ? 1 : 0;
#endif /* 0 */
}
