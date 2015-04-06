/*	$NetBSD: mace.c,v 1.20.6.1 2015/04/06 15:18:02 skrll Exp $	*/

/*
 * Copyright (c) 2003 Christopher Sekiya
 * Copyright (c) 2002,2003 Rafal K. Boni
 * Copyright (c) 2000 Soren S. Jorvang
 * All rights reserved.
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.NetBSD.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
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
 * O2 MACE
 *
 * The MACE is weird -- although it is a 32-bit device, writes only seem to
 * work properly if they are 64-bit-at-once writes (at least, out in ISA
 * space and probably MEC space -- the PCI stuff seems to be okay with _4).
 * Therefore, the _8* routines are used even though the top 32 bits are
 * thrown away.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mace.c,v 1.20.6.1 2015/04/06 15:18:02 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/callout.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/syslog.h>

#include <uvm/uvm_extern.h>

#include <sys/bus.h>
#include <machine/cpu.h>
#include <machine/locore.h>
#include <machine/autoconf.h>
#include <machine/machtype.h>

#include <sgimips/mace/macevar.h>
#include <sgimips/mace/macereg.h>
#include <sgimips/dev/crimevar.h>
#include <sgimips/dev/crimereg.h>

#include "locators.h"

#define MACE_NINTR 32 /* actually only 8, but interrupts are shared */

struct {
	unsigned int	irq;
	unsigned int	intrmask;
	int	(*func)(void *);
	void	*arg;
	struct evcnt evcnt;
	char	evname[32];
} maceintrtab[MACE_NINTR];

struct mace_softc {
	device_t sc_dev;

	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	bus_dma_tag_t dmat; /* 32KB ring buffers, 4KB segments, for ISA  */
	int nsegs;
	bus_dma_segment_t seg;
	bus_dmamap_t map;

	void *isa_ringbuffer;
};

static int	mace_match(device_t, cfdata_t, void *);
static void	mace_attach(device_t, device_t, void *);
static int	mace_print(void *, const char *);
static int	mace_search(device_t, cfdata_t, const int *, void *);

CFATTACH_DECL_NEW(mace, sizeof(struct mace_softc),
    mace_match, mace_attach, NULL, NULL);

static void mace_isa_bus_mem_init(bus_space_tag_t, void *);

static struct mips_bus_space	mace_isa_mbst;
bus_space_tag_t	mace_isa_memt = NULL;
static int mace_isa_init = 0;

#if defined(BLINK)
static callout_t mace_blink_ch;
static void	mace_blink(void *);
#endif

static int
mace_match(device_t parent, struct cfdata *match, void *aux)
{

	/*
	 * The MACE is in the O2.
	 */
	if (mach_type == MACH_SGI_IP32)
		return 1;

	return 0;
}

void
mace_init_bus(void)
{
	if (mace_isa_init == 1)
		return;
	mace_isa_init = 1;
	mace_isa_bus_mem_init(&mace_isa_mbst, NULL);
	mace_isa_memt = &mace_isa_mbst;
}
	
static void
mace_attach(device_t parent, device_t self, void *aux)
{
	struct mace_softc *sc = device_private(self);
	struct mainbus_attach_args *ma = aux;
	uint32_t scratch;

	sc->sc_dev = self;
#ifdef BLINK
	callout_init(&mace_blink_ch, 0);
#endif

	sc->iot = normal_memt;	/* for mace registers */
	sc->dmat = &sgimips_default_bus_dma_tag;

	if (bus_space_map(sc->iot, ma->ma_addr, 0,
	    BUS_SPACE_MAP_LINEAR, &sc->ioh))
		panic("mace_attach: could not allocate memory\n");

	aprint_normal("\n");

	aprint_debug("%s: isa sts %#"PRIx64"\n", device_xname(self),
	    bus_space_read_8(sc->iot, sc->ioh, MACE_ISA_INT_STATUS));
	aprint_debug("%s: isa msk %#"PRIx64"\n", device_xname(self),
	    bus_space_read_8(sc->iot, sc->ioh, MACE_ISA_INT_MASK));

	mace_init_bus();

	/*
	 * Turn on most ISA interrupts.  These are actually masked and
	 * registered via the CRIME, as the MACE ISA interrupt mask is
	 * really whacky and nigh on impossible to map to a sane autoconfig
	 * scheme.  We do, however, turn off the count/compare timer and RTC
	 * interrupts as they are unused and conflict with the PS/2
	 * keyboard and mouse interrupts.
	 */

	bus_space_write_8(sc->iot, sc->ioh, MACE_ISA_INT_MASK, 0xffff0aff);
	bus_space_write_8(sc->iot, sc->ioh, MACE_ISA_INT_STATUS, 0);

	/* set up LED for solid green or blink, if that's your fancy */
	scratch = bus_space_read_8(sc->iot, sc->ioh, MACE_ISA_FLASH_NIC_REG);
	scratch |= MACE_ISA_LED_RED;
	scratch &= ~(MACE_ISA_LED_GREEN);
	bus_space_write_8(sc->iot, sc->ioh, MACE_ISA_FLASH_NIC_REG, scratch);

#if defined(BLINK)
	mace_blink(sc);
#endif

	/* Initialize the maceintr elements to sane values */
	for (scratch = 0; scratch < MACE_NINTR; scratch++) {
		maceintrtab[scratch].func = NULL;
		maceintrtab[scratch].irq = 0;
	}

	config_search_ia(mace_search, self, "mace", NULL);
}


static int
mace_print(void *aux, const char *pnp)
{
	struct mace_attach_args *maa = aux;

	if (pnp != 0)
		return QUIET;

	if (maa->maa_offset != MACECF_OFFSET_DEFAULT)
		aprint_normal(" offset 0x%lx", maa->maa_offset);
	if (maa->maa_intr != MACECF_INTR_DEFAULT)
		aprint_normal(" intr %d", maa->maa_intr);
	if (maa->maa_offset != MACECF_INTRMASK_DEFAULT)
		aprint_normal(" intrmask 0x%x", maa->maa_intrmask);

	return UNCONF;
}

static int
mace_search(device_t parent, struct cfdata *cf, const int *ldesc, void *aux)
{
	struct mace_softc *sc = device_private(parent);
	struct mace_attach_args maa;
	int tryagain;

	do {
		maa.maa_offset = cf->cf_loc[MACECF_OFFSET];
		maa.maa_intr = cf->cf_loc[MACECF_INTR];
		maa.maa_intrmask = cf->cf_loc[MACECF_INTRMASK];
		maa.maa_st = normal_memt;
		maa.maa_sh = sc->ioh;	/* XXX */
		maa.maa_dmat = &sgimips_default_bus_dma_tag;
		maa.isa_ringbuffer = sc->isa_ringbuffer;

		tryagain = 0;
		if (config_match(parent, cf, &maa) > 0) {
			config_attach(parent, cf, &maa, mace_print);
			tryagain = (cf->cf_fstate == FSTATE_STAR);
		}

	} while (tryagain);

	return 0;
}

void *
mace_intr_establish(int intr, int level, int (*func)(void *), void *arg)
{
	int i;

	if (intr < 0 || intr >= 16)
		panic("invalid interrupt number");

	for (i = 0; i < MACE_NINTR; i++)
		if (maceintrtab[i].func == NULL) {
		        maceintrtab[i].func = func;
		        maceintrtab[i].arg = arg;
			maceintrtab[i].irq = (1 << intr);
			maceintrtab[i].intrmask = level;
			snprintf(maceintrtab[i].evname,
			    sizeof(maceintrtab[i].evname),
			    "intr %d level 0x%x", intr, level);
			evcnt_attach_dynamic(&maceintrtab[i].evcnt,
			    EVCNT_TYPE_INTR, NULL,
			    "mace", maceintrtab[i].evname);
			break;
		}

	crime_intr_mask(intr);
	aprint_debug("mace: established interrupt %d (level %x)\n",
	    intr, level);
	return (void *)&maceintrtab[i];
}

void
mace_intr_disestablish(void *cookie)
{
	int intr = -1, level = 0, irq = 0, i;

	for (i = 0; i < MACE_NINTR; i++)
		if (&maceintrtab[i] == cookie) {
			evcnt_detach(&maceintrtab[i].evcnt);
			for (intr = 0;
			    maceintrtab[i].irq == (1 << intr); intr++);
			level = maceintrtab[i].intrmask;
			irq = maceintrtab[i].irq;

			maceintrtab[i].irq = 0;
			maceintrtab[i].intrmask = 0;
		        maceintrtab[i].func = NULL;
		        maceintrtab[i].arg = NULL;
			memset(&maceintrtab[i].evcnt, 0, sizeof (struct evcnt));
			memset(&maceintrtab[i].evname, 0,
			    sizeof (maceintrtab[i].evname));
			break;
		}
	if (intr == -1)
		panic("mace: lost maceintrtab");

	/* do not do an unmask when irq is shared. */
	for (i = 0; i < MACE_NINTR; i++)
		if (&maceintrtab[i].func != NULL && maceintrtab[i].irq == irq)
			break;
	if (i == MACE_NINTR)
		crime_intr_unmask(intr);
	aprint_debug("mace: disestablished interrupt %d (level %x)\n",
	    intr, level);
}

void
mace_intr(int irqs)
{
	uint64_t isa_irq;
	int i;

	/* irq 4 is the ISA cascade interrupt.  Must handle with care. */
	if (irqs & (1 << 4)) {
		isa_irq = mips3_ld((volatile uint64_t *)MIPS_PHYS_TO_KSEG1(MACE_BASE
		    + MACE_ISA_INT_STATUS));
		for (i = 0; i < MACE_NINTR; i++) {
			if ((maceintrtab[i].irq == (1 << 4)) &&
			    (isa_irq & maceintrtab[i].intrmask)) {
		  		(maceintrtab[i].func)(maceintrtab[i].arg);
				maceintrtab[i].evcnt.ev_count++;
	        	}
		}
		irqs &= ~(1 << 4);
	}

	for (i = 0; i < MACE_NINTR; i++)
		if ((irqs & maceintrtab[i].irq)) {
		  	(maceintrtab[i].func)(maceintrtab[i].arg);
			maceintrtab[i].evcnt.ev_count++;
		}
}

#if defined(BLINK)
static void
mace_blink(void *self)
{
	struct mace_softc *sc = device_private(self);
	register int s;
	int value;

	s = splhigh();
	value = bus_space_read_8(sc->iot, sc->ioh, MACE_ISA_FLASH_NIC_REG);
	value ^= MACE_ISA_LED_GREEN;
	bus_space_write_8(sc->iot, sc->ioh, MACE_ISA_FLASH_NIC_REG, value);
	splx(s);
	/*
	 * Blink rate is:
	 *      full cycle every second if completely idle (loadav = 0)
	 *      full cycle every 2 seconds if loadav = 1
	 *      full cycle every 3 seconds if loadav = 2
	 * etc.
	 */
	s = (((averunnable.ldavg[0] + FSCALE) * hz) >> (FSHIFT + 1));
	callout_reset(&mace_blink_ch, s, mace_blink, sc);

}
#endif

#define CHIP	   		mace_isa
#define	CHIP_MEM		/* defined */
#define CHIP_ALIGN_STRIDE	8
#define CHIP_ACCESS_SIZE	8
#define	CHIP_W1_BUS_START(v)	0x00000000UL
#define CHIP_W1_BUS_END(v)	0xffffffffUL
#define	CHIP_W1_SYS_START(v)	0x00000000UL
#define	CHIP_W1_SYS_END(v)	0xffffffffUL

#include <mips/mips/bus_space_alignstride_chipdep.c>
