/*	$NetBSD: pci_machdep.c,v 1.2.2.3 2002/04/01 07:42:54 nathanw Exp $ */

/*
 * Copyright (c) 1999, 2000 Matthew R. Green
 * Copyright (c) 2001 Valeriy E. Ushakov
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Machine-dependent PCI bits for PCI controller in microSPARC-IIep.
 * References are to the microSPARC-IIep manual unless noted otherwise.
 */

#if defined(DEBUG) && !defined(SPARC_PCI_DEBUG)
#define SPARC_PCI_DEBUG
#endif

#ifdef SPARC_PCI_DEBUG
#define SPDB_CONF	0x01
#define SPDB_INTR	0x04
#define SPDB_INTMAP	0x08
#define SPDB_INTFIX	0x10
#define SPDB_PROBE	0x20
int sparc_pci_debug = 0;
#define DPRINTF(l, s)	do { if (sparc_pci_debug & l) printf s; } while (0)
#else
#define DPRINTF(l, s)
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/openfirm.h>

#include <machine/ctlreg.h>
#include <sparc/sparc/asm.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include <dev/ofw/ofw_pci.h>

#include <sparc/sparc/msiiepreg.h>
#include <sparc/sparc/msiiepvar.h>

/*
 * Table 9-1 (p. 129).
 *   Configuration space access.  This goes via MMU bypass ASI.
 */
#define PCI_MODE1_ADDRESS_REG_PA	0x30080000
#define	PCI_MODE1_DATA_REG_PA		0x300a0000

/*
 * Footnote 1 in Table 9-1 (p. 129):
 * 
 *   Three least significant bits of the configuration data space
 *   access must match those of the configuration address space access.
 */
#define PCI_MODE1_DATA_REG_MASK	0x7


/*
 * PROMs in ms-IIep systems just lie about PCI and EBus interrupts, so
 * we just hardcode the wiring based on the model we are running on.
 * Probably we can do some forth hacking in boot loader's prompatch
 * (that's what it was introduced for), but for now it's way more
 * simple to just hardcode it here.
 */


struct mspcic_pci_intr_wiring {
	u_int		mpiw_bus;
	u_int		mpiw_device;
	u_int		mpiw_function;
	pci_intr_line_t	mpiw_line;
};

static struct mspcic_pci_intr_wiring krups_pci_intr_wiring[] = {
	{ 0, 0, 1,    1 },	/* ethernet */
	{ 0, 1, 0,    2 },	/* vga */
};


struct mspcic_known_model {
	const char *model;
	struct mspcic_pci_intr_wiring *map;
	int mapsize;
};

#define MSPCIC_MODEL_WIRING(name,map) \
	{ name, map, sizeof(map)/sizeof(map[0]) }

static struct mspcic_known_model mspcic_known_models[] = {
	MSPCIC_MODEL_WIRING("SUNW,501-4267", krups_pci_intr_wiring),
	{ NULL, NULL, 0}
};


static struct mspcic_pci_intr_wiring *wiring_map;
static int wiring_map_size;


void
pci_attach_hook(parent, self, pba)
	struct device *parent;
	struct device *self;
	struct pcibus_attach_args *pba;
{
	struct mspcic_known_model *p;
	char buf[32];
	char *model;

	model = PROM_getpropstringA(prom_findroot(), "model",
				    buf, sizeof(buf));
	if (model == NULL)
		panic("pci_attach_hook: no \"model\" property");

	printf(": model %s", model);

	for (p = mspcic_known_models; p->model != NULL; ++p)
		if (strcmp(model, p->model) == 0) {
			printf(": interrupt wiring known");
			wiring_map = p->map;
			wiring_map_size = p->mapsize;
			return;
		}

	/* not found */
	printf(": don't know how interrupts are wired\n");
	panic("pci_attach_hook: unknown model %s", model);
}


int
pci_bus_maxdevs(pc, busno)
	pci_chipset_tag_t pc;
	int busno;
{

	return (32);
}


pcitag_t
pci_make_tag(pc, b, d, f)
	pci_chipset_tag_t pc;
	int b;
	int d;
	int f;
{
	struct mspcic_softc *sc = (struct mspcic_softc *)pc->cookie;
	pcitag_t tag;
	int node, len;
#ifdef SPARC_PCI_DEBUG
	char name[80];

	memset(name, 0, sizeof(name));
#endif

	tag = PCITAG_CREATE(-1, b, d, f);
	if (b >= 256 || d >= 32 || f >= 8) {
		printf("pci_make_tag: bad request %d/%d/%d\n", b, d, f);
		return (tag); /* a dead one */
	}

	/*
	 * XXX: OFW 3.11 doesn't have "bus-range" property on its
	 * "/pci" node.  As a workaround we start with the first child
	 * of "/pci" instead of matching the bus number against the
	 * "bus-range" of the "/pci" node.
	 *
	 * Traverse all peers until we find the node.
	 */
	for (node = OF_child(sc->sc_node); node != 0; node = OF_peer(node)) {
		struct ofw_pci_register reg;
		u_int32_t busrange[2];

#ifdef SPARC_PCI_DEBUG
		if (sparc_pci_debug & SPDB_PROBE) {
			OF_getprop(node, "name", &name, sizeof(name));
			printf("> checking node %x %s\n", node, name);
		}
#endif
		/*
		 * Check for PCI-PCI bridges.  If the device we want is
		 * in the bus-range for that bridge, work our way down.
		 */
		while ((OF_getprop(node, "bus-range", (void *)&busrange,
				   sizeof(busrange)) == sizeof(busrange))
		       && (b >= busrange[0] && b <= busrange[1]))
		{
			/* go down one level */
			node = OF_child(node);
#ifdef SPARC_PCI_DEBUG
			if (sparc_pci_debug & SPDB_PROBE) {
				OF_getprop(node, "name", &name, sizeof(name));
				printf("> going down to node %x %s\n",
					node, name);
			}
#endif
		}

		/* 
		 * We only really need the first `reg' property. 
		 *
		 * For simplicity, we'll query the `reg' when we
		 * need it.  Otherwise we could malloc() it, but
		 * that gets more complicated.
		 */
		len = OF_getproplen(node, "reg");
		if (len < sizeof(reg))
			continue;
		if (OF_getprop(node, "reg", (void *)&reg, sizeof(reg)) != len)
			panic("pci_probe_bus: OF_getprop len botch");

		if (b != OFW_PCI_PHYS_HI_BUS(reg.phys_hi))
			continue;
		if (d != OFW_PCI_PHYS_HI_DEVICE(reg.phys_hi))
			continue;
		if (f != OFW_PCI_PHYS_HI_FUNCTION(reg.phys_hi))
			continue;

		/* Got a match */
		tag = PCITAG_CREATE(node, b, d, f);

		/* Enable all the different spaces for this device */
		pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG,
			PCI_COMMAND_MASTER_ENABLE
			| PCI_COMMAND_MEM_ENABLE | PCI_COMMAND_IO_ENABLE);
		DPRINTF(SPDB_PROBE, ("> found node %x %s\n", node, name));
		return (tag);
	}

	/* No device found - return a dead tag */
	return (tag);
}


void
pci_decompose_tag(pc, tag, bp, dp, fp)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int *bp, *dp, *fp;
{
	if (*bp != NULL)
		*bp = PCITAG_BUS(tag);
	if (*dp != NULL)
		*dp = PCITAG_DEV(tag);
	if (*fp != NULL)
		*fp = PCITAG_FUN(tag);
}


pcireg_t
pci_conf_read(pc, tag, reg)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int reg;
{
	u_int32_t mode1_addr;
	u_int32_t mode1_data_reg_pa;
	pcireg_t val = (pcireg_t)~0;

	DPRINTF(SPDB_CONF,
		("pci_conf_read:  tag=%x.%x (%d/%d/%d), reg=%02x; ",
		 PCITAG_NODE(tag), PCITAG_OFFSET(tag),
		 PCITAG_BUS(tag), PCITAG_DEV(tag), PCITAG_FUN(tag),
		 reg));

#ifdef DIAGNOSTIC
	if (reg & 0x3)
		panic("pci_conf_read: reg %x unaligned", reg);
	if (reg & ~0xff)
		panic("pci_conf_read: reg %x out of range", reg);
#endif

	if (PCITAG_NODE(tag) == -1) {
		DPRINTF(SPDB_CONF, ("\n"));
		return (val);
	}

	mode1_addr = PCITAG_OFFSET(tag) | reg;
	mode1_data_reg_pa = PCI_MODE1_DATA_REG_PA
		| (reg & PCI_MODE1_DATA_REG_MASK);

	/* 
	 * NB: we run in endian-swapping mode, so we don't need to
	 * convert mode1_addr and val.
	 */
	sta(PCI_MODE1_ADDRESS_REG_PA, ASI_BYPASS, mode1_addr);
	val = lda(mode1_data_reg_pa, ASI_BYPASS);

	DPRINTF(SPDB_CONF, ("reading %08x\n", (u_int)val));

	return (val);
}


void
pci_conf_write(pc, tag, reg, data)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int reg;
	pcireg_t data;
{
	u_int32_t mode1_addr;
	u_int32_t mode1_data_reg_pa;

	DPRINTF(SPDB_CONF,
		("pci_conf_write: tag=%x.%x (%d/%d/%d); reg=%02x; ",
		 PCITAG_NODE(tag), PCITAG_OFFSET(tag),
		 PCITAG_BUS(tag), PCITAG_DEV(tag), PCITAG_FUN(tag),
		 reg));

#ifdef DIAGNOSTIC
	if (reg & 0x3)
		panic("pci_conf_write: reg %x unaligned", reg);
	if (reg & ~0xff)
		panic("pci_conf_write: reg %x out of range", reg);
#endif

	if (PCITAG_NODE(tag) == -1) {
		DPRINTF(SPDB_CONF, ("\n"));
		return;
	}

	mode1_addr = PCITAG_OFFSET(tag) | reg;
	mode1_data_reg_pa = PCI_MODE1_DATA_REG_PA
		| (reg & PCI_MODE1_DATA_REG_MASK);

	DPRINTF(SPDB_CONF, ("writing %08x\n", data));

	/* 
	 * NB: we run in endian-swapping mode, so we don't need to
	 * convert mode1_addr and data.
	 */
	sta(PCI_MODE1_ADDRESS_REG_PA, ASI_BYPASS, mode1_addr);
	sta(mode1_data_reg_pa, ASI_BYPASS, data);
}


/* ======================================================================
 *
 *	       PCI bus interrupt manipulation functions
 */

int
pci_intr_map(pa, ihp)
	struct pci_attach_args *pa;
	pci_intr_handle_t *ihp;
{
	int i;

	DPRINTF(SPDB_INTMAP,
		("pci_intr_map(%d/%d/%d) -> ",
		 pa->pa_bus, pa->pa_device, pa->pa_function));

	for (i = 0; i < wiring_map_size; ++i) {
		struct mspcic_pci_intr_wiring *w = &wiring_map[i];

		if (pa->pa_bus == w->mpiw_bus
		    && pa->pa_device == w->mpiw_device
		    && pa->pa_function == w->mpiw_function)
		{
			DPRINTF(SPDB_INTMAP, ("line %d\n", w->mpiw_line));
			*ihp = w->mpiw_line;
			return (0);
		}
	}

	DPRINTF(SPDB_INTMAP, ("not found\n"));
	return (-1);
}


const char *
pci_intr_string(pc, ih)
	pci_chipset_tag_t pc;
	pci_intr_handle_t ih;
{
	static char str[16];
	int pil;

	pil = mspcic_assigned_interrupt(ih);
	sprintf(str, "line %d (pil %d)", ih, pil);
	return (str);
}


const struct evcnt *
pci_intr_evcnt(pc, ih)
	pci_chipset_tag_t pc;
	pci_intr_handle_t ih;
{

	/* XXX for now, no evcnt parent reported */
	return NULL;
}


void *
pci_intr_establish(pc, ih, level, func, arg)
	pci_chipset_tag_t pc;
	pci_intr_handle_t ih;
	int level;
	int (*func)(void *);
	void *arg;
{
	struct mspcic_softc *sc = (struct mspcic_softc *)pc->cookie;
	void *cookie = NULL;

	DPRINTF(SPDB_INTR,
		("pci_intr_establish(line %d, ipl %d)\n", ih, level));

	cookie = bus_intr_establish(sc->sc_memt, ih, level, 0, func, arg);

	/*
	 * TODO: to implement pci_intr_disestablish we need to capture
	 * the 'intrhand' returned by bus_intr_establish above and the
	 * pil the handler was established for, but we don't need to
	 * disestablish pci interrupts for now (and I doubt we will),
	 * so why bother.
	 */

	DPRINTF(SPDB_INTR,
		("pci_intr_establish: returning handle %p\n", cookie));
	return (cookie);
}


void
pci_intr_disestablish(pc, cookie)
	pci_chipset_tag_t pc;
	void *cookie;
{

	DPRINTF(SPDB_INTR, ("pci_intr_disestablish: cookie %p\n", cookie));
	panic("pci_intr_disestablish: not implemented");
}
