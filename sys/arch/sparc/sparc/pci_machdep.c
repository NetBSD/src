/*	$NetBSD: pci_machdep.c,v 1.21 2015/10/02 05:22:52 msaitoh Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pci_machdep.c,v 1.21 2015/10/02 05:22:52 msaitoh Exp $");

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
#define DPRINTF(l, s)	do { 			\
	if (sparc_pci_debug & (l))		\
		printf s;			\
} while (/* CONSTCOND */ 0)
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

#include <sys/bus.h>
#include <machine/autoconf.h>

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
 * XXX: Unknown mappings for PCI slots set to line 8.
 */

struct mspcic_pci_intr_wiring {
	u_int		mpiw_bus;
	u_int		mpiw_device;
	u_int		mpiw_function;
	pci_intr_line_t	mpiw_line[4];	/* Int A (0) - Int D (3) */
};

static struct mspcic_pci_intr_wiring krups_pci_intr_wiring[] = {
	{ 0, 0, 1,    { 1, 0, 0, 0 } },	/* ethernet */
	{ 0, 1, 0,    { 2, 0, 0, 0 } },	/* vga */
};

static struct mspcic_pci_intr_wiring espresso_pci_intr_wiring[] = {
	{ 0,  0, 1,    { 1, 0, 0, 0 } },	/* ethernet */
	{ 0,  1, 0,    { 2, 0, 0, 0 } },	/* vga */
	{ 0,  2, 0,    { 6, 7, 8, 8 } },	/* pci slot1 */
	{ 0,  2, 1,    { 6, 7, 8, 8 } },	/* pci slot1 */
	{ 0,  2, 2,    { 6, 7, 8, 8 } },	/* pci slot1 */
	{ 0,  2, 3,    { 6, 7, 8, 8 } },	/* pci slot1 */
	{ 0,  2, 4,    { 6, 7, 8, 8 } },	/* pci slot1 */
	{ 0,  2, 5,    { 6, 7, 8, 8 } },	/* pci slot1 */
	{ 0,  2, 6,    { 6, 7, 8, 8 } },	/* pci slot1 */
	{ 0,  2, 7,    { 6, 7, 8, 8 } },	/* pci slot1 */
	{ 0,  3, 0,    { 7, 8, 8, 8 } },	/* pci slot2 */
	{ 0,  3, 1,    { 7, 8, 8, 8 } },	/* pci slot2 */
	{ 0,  3, 2,    { 7, 8, 8, 8 } },	/* pci slot2 */
	{ 0,  3, 3,    { 7, 8, 8, 8 } },	/* pci slot2 */
	{ 0,  3, 4,    { 7, 8, 8, 8 } },	/* pci slot2 */
	{ 0,  3, 5,    { 7, 8, 8, 8 } },	/* pci slot2 */
	{ 0,  3, 6,    { 7, 8, 8, 8 } },	/* pci slot2 */
	{ 0,  3, 7,    { 7, 8, 8, 8 } },	/* pci slot2 */
	{ 0,  7, 0,    { 4, 0, 0, 0 } },	/* isa */
	{ 0, 16, 0,    { 5, 0, 0, 0 } },	/* eide */
	{ 0, 20, 0,    { 5, 0, 0, 0 } },	/* usb */
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
	MSPCIC_MODEL_WIRING("SUNW,375-0059", espresso_pci_intr_wiring),
	{ NULL, NULL, 0}
};


static struct mspcic_pci_intr_wiring *wiring_map;
static int wiring_map_size;


void
pci_attach_hook(device_t parent, device_t self,
		struct pcibus_attach_args *pba)
{
	struct mspcic_known_model *p;
	char buf[32];
	char *model;

	/* We only need to run once (root PCI bus is 0) */
	if (pba->pba_bus != 0)
		return;

	model = prom_getpropstringA(prom_findroot(), "model",
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
pci_bus_maxdevs(pci_chipset_tag_t pc, int busno)
{

	return 32;
}


pcitag_t
pci_make_tag(pci_chipset_tag_t pc, int b, int d, int f)
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
		return tag;	/* a dead one */
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
		uint32_t busrange[2];
		int class;
		pcireg_t busdata;
		pcitag_t bustag;

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
		OF_getprop(node, "reg", (void *)&reg, sizeof(reg));

		/*
		 * Check for (OFW unconfigured) bridges that we fixed up.
		 * We'll set this top-level bridge's node in the tag,
		 * so that we can use it later for interrupt wiring.
		 */
		if (b > 0) {
			len = OF_getproplen(node, "class-code");
			if (!len)
				continue;
			OF_getprop(node, "class-code", &class, len);
			if (IS_PCI_BRIDGE(class)) {
				bustag = PCITAG_CREATE(node,
				    OFW_PCI_PHYS_HI_BUS(reg.phys_hi),
				    OFW_PCI_PHYS_HI_DEVICE(reg.phys_hi),
				    OFW_PCI_PHYS_HI_FUNCTION(reg.phys_hi));
				busdata = pci_conf_read(NULL, bustag,
				    PCI_BRIDGE_BUS_REG);
				if (b != ((busdata >> 8) & 0xff))
					continue;

#ifdef SPARC_PCI_DEBUG
				if (sparc_pci_debug & SPDB_PROBE) {
					OF_getprop(node, "name", &name,
					    sizeof(name));
					printf("> matched device behind node "
					    "%x %s (bus %d)\n", node, name, b);
				}
#endif
			} else
				continue;
		} else {
			if (b != OFW_PCI_PHYS_HI_BUS(reg.phys_hi))
				continue;
			if (d != OFW_PCI_PHYS_HI_DEVICE(reg.phys_hi))
				continue;
			if (f != OFW_PCI_PHYS_HI_FUNCTION(reg.phys_hi))
				continue;
		}

		/* Got a match */
		tag = PCITAG_CREATE(node, b, d, f);
		DPRINTF(SPDB_PROBE, ("> found node %x %s\n", node, name));
		return tag;
	}

	/* No device found - return a dead tag */
	return tag;
}


void
pci_decompose_tag(pci_chipset_tag_t pc, pcitag_t tag,
		  int *bp, int *dp, int *fp)
{

	if (bp != NULL)
		*bp = PCITAG_BUS(tag);
	if (dp != NULL)
		*dp = PCITAG_DEV(tag);
	if (fp != NULL)
		*fp = PCITAG_FUN(tag);
}


pcireg_t
pci_conf_read(pci_chipset_tag_t pc, pcitag_t tag, int reg)
{
	uint32_t mode1_addr;
	uint32_t mode1_data_reg_pa;
	uint32_t val;

	DPRINTF(SPDB_CONF,
		("pci_conf_read:  tag=%x.%x (%d/%d/%d), reg=%02x; ",
		 PCITAG_NODE(tag), PCITAG_OFFSET(tag),
		 PCITAG_BUS(tag), PCITAG_DEV(tag), PCITAG_FUN(tag),
		 reg));

#ifdef DIAGNOSTIC
	if (reg & 0x3)
		panic("pci_conf_read: reg %x unaligned", reg);
#endif

	if (PCITAG_NODE(tag) == -1) {
		DPRINTF(SPDB_CONF, ("\n"));
		return ~0;
	}

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return ~0;

	mode1_addr = PCITAG_OFFSET(tag) | reg;
	mode1_data_reg_pa = PCI_MODE1_DATA_REG_PA
		| (reg & PCI_MODE1_DATA_REG_MASK);

	sta(PCI_MODE1_ADDRESS_REG_PA, ASI_BYPASS, htole32(mode1_addr));
	val = le32toh(lda(mode1_data_reg_pa, ASI_BYPASS));

	DPRINTF(SPDB_CONF, ("reading %08x\n", val));

	return val;
}


void
pci_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t data)
{
	uint32_t mode1_addr;
	uint32_t mode1_data_reg_pa;

	DPRINTF(SPDB_CONF,
		("pci_conf_write: tag=%x.%x (%d/%d/%d); reg=%02x; ",
		 PCITAG_NODE(tag), PCITAG_OFFSET(tag),
		 PCITAG_BUS(tag), PCITAG_DEV(tag), PCITAG_FUN(tag),
		 reg));

#ifdef DIAGNOSTIC
	if (reg & 0x3)
		panic("pci_conf_write: reg %x unaligned", reg);
#endif

	if (PCITAG_NODE(tag) == -1) {
		DPRINTF(SPDB_CONF, ("\n"));
		return;
	}

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return;

	mode1_addr = PCITAG_OFFSET(tag) | reg;
	mode1_data_reg_pa = PCI_MODE1_DATA_REG_PA
		| (reg & PCI_MODE1_DATA_REG_MASK);

	DPRINTF(SPDB_CONF, ("writing %08x\n", data));

	sta(PCI_MODE1_ADDRESS_REG_PA, ASI_BYPASS, htole32(mode1_addr));
	sta(mode1_data_reg_pa, ASI_BYPASS, htole32(data));
}


/* ======================================================================
 *
 *	       PCI bus interrupt manipulation functions
 */

int
pci_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	int i, node;
	pcitag_t tag;
	pcireg_t val;
	pci_intr_pin_t pin;

	DPRINTF(SPDB_INTMAP,
		("pci_intr_map(%d/%d/%d) -> ",
		 pa->pa_bus, pa->pa_device, pa->pa_function));

	tag = pci_make_tag(pa->pa_pc, pa->pa_bus, pa->pa_device,
	    pa->pa_function);
	node = PCITAG_NODE(tag);
	val = pci_conf_read(NULL, tag, PCI_INTERRUPT_REG);
	pin = PCI_INTERRUPT_PIN(val);

	/*
	 * Pin should be A(1) to D(4) - use values 0 to 3 respectively to
	 * represent them.  Built-in devices might show pin 0, so assume
	 * pin A for those - the static wiring map has the correct line.
	 */
	if (pin)
		pin -= 1;

	for (i = 0; i < wiring_map_size; ++i) {
		struct mspcic_pci_intr_wiring *w = &wiring_map[i];

		/* Device on PCI bus 0 */
		if (pa->pa_bus == w->mpiw_bus
		    && pa->pa_device == w->mpiw_device
		    && pa->pa_function == w->mpiw_function)
		{
			if (w->mpiw_line[pin] > 7) {
				DPRINTF(SPDB_INTMAP, ("not mapped\n"));
				return -1;
			}
			DPRINTF(SPDB_INTMAP, ("pin %c line %d\n", 'A' + pin,
			    w->mpiw_line[pin]));
			*ihp = w->mpiw_line[pin];
			return 0;
		/* Device on other PCI bus - find top-level bridge device */
		} else if (pa->pa_bus) {
			struct ofw_pci_register reg;

			OF_getprop(node, "reg", (void *)&reg, sizeof(reg));
			if (OFW_PCI_PHYS_HI_BUS(reg.phys_hi) == w->mpiw_bus
			    && OFW_PCI_PHYS_HI_DEVICE(reg.phys_hi)
			    == w->mpiw_device
			    && OFW_PCI_PHYS_HI_FUNCTION(reg.phys_hi)
			    == w->mpiw_function) {
				int j;

				/* PCI bridge interrupt swizzle */
				for (j = 0; j < PCI_INTERRUPT_LINE(val); j++)
					pin = (pin + (pa->pa_device % 4)) % 4;

				if (w->mpiw_line[pin] > 7) {
					DPRINTF(SPDB_INTMAP, ("pin %c "
					    "not mapped\n", pin));
					return -1;
				}
				DPRINTF(SPDB_INTMAP, ("pin %c line %d "
				    "via bridge (%d/%d/%d) depth %d\n",
				    'A' + pin, w->mpiw_line[pin],
				    w->mpiw_bus, w->mpiw_device,
				    w->mpiw_function,
				    PCI_INTERRUPT_LINE(val)));
				*ihp = w->mpiw_line[pin];
				return 0;
			}
		}
	}

	DPRINTF(SPDB_INTMAP, ("not found\n"));
	return -1;
}


const char *
pci_intr_string(pci_chipset_tag_t pc, pci_intr_handle_t ih, char *buf,
    size_t len)
{
	int pil;

	pil = mspcic_assigned_interrupt(ih);
	snprintf(buf, len, "line %d (pil %d)", ih, pil);
	return buf;
}


const struct evcnt *
pci_intr_evcnt(pci_chipset_tag_t pc, pci_intr_handle_t ih)
{

	/* XXX for now, no evcnt parent reported */
	return NULL;
}

int
pci_intr_setattr(pci_chipset_tag_t pc, pci_intr_handle_t *ih,
		 int attr, uint64_t data)
{

	switch (attr) {
	case PCI_INTR_MPSAFE:
		return 0;
	default:
		return ENODEV;
	}
}

void *
pci_intr_establish(pci_chipset_tag_t pc, pci_intr_handle_t ih,
		   int level, int (*func)(void *), void *arg)
{
	struct mspcic_softc *sc = (struct mspcic_softc *)pc->cookie;
	void *cookie;

	DPRINTF(SPDB_INTR,
		("pci_intr_establish(line %d, ipl %d)\n", ih, level));

	cookie = bus_intr_establish(sc->sc_memt, ih, level, func, arg);

	/*
	 * TODO: to implement pci_intr_disestablish we need to capture
	 * the 'intrhand' returned by bus_intr_establish above and the
	 * pil the handler was established for, but we don't need to
	 * disestablish pci interrupts for now (and I doubt we will),
	 * so why bother.
	 */

	DPRINTF(SPDB_INTR,
		("pci_intr_establish: returning handle %p\n", cookie));
	return cookie;
}


void
pci_intr_disestablish(pci_chipset_tag_t pc, void *cookie)
{

	DPRINTF(SPDB_INTR, ("pci_intr_disestablish: cookie %p\n", cookie));
	panic("pci_intr_disestablish: not implemented");
}
