/*	$NetBSD: pci_machdep.c,v 1.9 2000/06/08 23:03:17 eeh Exp $	*/

/*
 * Copyright (c) 1999, 2000 Matthew R. Green
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
 * functions expected by the MI PCI code.
 */

#ifdef DEBUG
#define SPDB_CONF	0x01
#define SPDB_INTR	0x04
#define SPDB_INTMAP	0x08
#define SPDB_INTFIX	0x10
int sparc_pci_debug = 0x0;
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

#include <vm/vm.h>
#include <vm/vm_kern.h>

#define _SPARC_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/autoconf.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include <sparc64/dev/iommureg.h>
#include <sparc64/dev/iommuvar.h>
#include <sparc64/dev/psychoreg.h>
#include <sparc64/dev/psychovar.h>

/* this is a base to be copied */
struct sparc_pci_chipset _sparc_pci_chipset = {
	NULL,
};

/*
 * functions provided to the MI code.
 */

void
pci_attach_hook(parent, self, pba)
	struct device *parent;
	struct device *self;
	struct pcibus_attach_args *pba;
{
	pci_chipset_tag_t pc = pba->pba_pc;
	struct psycho_pbm *pp = pc->cookie;
	struct psycho_registers *pr;
	pcitag_t tag;
	char *name, *devtype;
	u_int32_t hi, mid, lo, intr;
	u_int32_t dev, fn, bus;
	int node, i, n, *ip, *ap;

	DPRINTF((SPDB_INTFIX|SPDB_INTMAP), ("\npci_attach_hook:"));

	/*
	 * ok, here we look in the OFW for each PCI device and fix it's
	 * "interrupt line" register to be useful.
	 */

	for (node = firstchild(pc->node); node; node = nextsibling(node)) {
		pr = NULL;
		ip = ap = NULL;

		/* 
		 * ok, for each child we get the "interrupts" property,
		 * which contains a value to match against later.
		 * XXX deal with multiple "interrupts" values XXX.
		 * then we get the "assigned-addresses" property which
		 * contains, in the first entry, the PCI bus, device and
		 * function associated with this node, which we use to
		 * generate a pcitag_t to use pci_conf_read() and
		 * pci_conf_write().  next, we get the 'reg" property
		 * which is structured like the following:
		 *	u_int32_t	phys_hi;
		 *	u_int32_t	phys_mid;
		 *	u_int32_t	phys_lo;
		 *	u_int32_t	size_hi;
		 *	u_int32_t	size_lo;
		 * we mask these values with the "interrupt-map-mask"
		 * property of our parent and them compare with each
		 * entry in the "interrupt-map" property (also of our
		 * parent) which is structred like the following:
		 *	u_int32_t	phys_hi;
		 *	u_int32_t	phys_mid;
		 *	u_int32_t	phys_lo;
		 *	u_int32_t	intr;
		 *	int32_t		child_node;
		 *	u_int32_t	child_intr;
		 * if there is an exact match with phys_hi, phys_mid,
		 * phys_lo and the interrupt, we have a match and we
		 * know that this interrupt's value is really the
		 * child_intr of the interrupt map entry.  we put this
		 * into the PCI interrupt line register so that when
		 * the driver for this node wants to attach, we know
		 * it's INO already.
		 */

		name = getpropstring(node, "name");
		DPRINTF((SPDB_INTFIX|SPDB_INTMAP), ("\n\tnode %x name `%s'", node, name));
		devtype = getpropstring(node, "device_type");
		DPRINTF((SPDB_INTFIX|SPDB_INTMAP), (" devtype `%s':", devtype));

		/* ignore PCI bridges, we'll get them later */
		if (strcmp(devtype, "pci") == 0)
			continue;

		/* if there isn't any "interrupts" then we don't care to fix it */
		ip = NULL;
		if (getprop(node, "interrupts", sizeof(int), &n, (void **)&ip))
			continue;
		DPRINTF(SPDB_INTFIX, (" got interrupts"));
		
		/* and if there isn't an "assigned-addresses" we can't find b/d/f */
		if (getprop(node, "assigned-addresses", sizeof(int), &n,
		    (void **)&ap))
			goto clean1;
		DPRINTF(SPDB_INTFIX, (" got assigned-addresses"));

		/* ok, and now the "reg" property, so we know what we're talking about. */
		if (getprop(node, "reg", sizeof(*pr), &n,
		    (void **)&pr))
			goto clean2;
		DPRINTF(SPDB_INTFIX, (" got reg"));

		bus = TAG2BUS(ap[0]);
		dev = TAG2DEV(ap[0]);
		fn = TAG2FN(ap[0]);

		DPRINTF(SPDB_INTFIX, ("; bus %u dev %u fn %u", bus, dev, fn));

		tag = pci_make_tag(pc, bus, dev, fn);

		DPRINTF(SPDB_INTFIX, ("; tag %08x\n\t; reg: hi %x mid %x lo %x intr %x", tag, pr->phys_hi, pr->phys_mid, pr->phys_lo, *ip));
		DPRINTF(SPDB_INTFIX, ("\n\t; intmapmask: hi %x mid %x lo %x intr %x", pp->pp_intmapmask.phys_hi, pp->pp_intmapmask.phys_mid,
										      pp->pp_intmapmask.phys_lo, pp->pp_intmapmask.intr));

		hi = pr->phys_hi & pp->pp_intmapmask.phys_hi;
		mid = pr->phys_mid & pp->pp_intmapmask.phys_mid;
		lo = pr->phys_lo & pp->pp_intmapmask.phys_lo;
		intr = *ip & pp->pp_intmapmask.intr;

		DPRINTF(SPDB_INTFIX, ("\n\t; after: hi %x mid %x lo %x intr %x", hi, mid, lo, intr));

		for (i = 0; i < pp->pp_nintmap; i++) {
			DPRINTF(SPDB_INTFIX, ("\n\t\tmatching for: hi %x mid %x lo %x intr %x", pp->pp_intmap[i].phys_hi, pp->pp_intmap[i].phys_mid,
												pp->pp_intmap[i].phys_lo, pp->pp_intmap[i].intr));

			if (pp->pp_intmap[i].phys_hi != hi ||
			    pp->pp_intmap[i].phys_mid != mid ||
			    pp->pp_intmap[i].phys_lo != lo ||
			    pp->pp_intmap[i].intr != intr)
				continue;
			DPRINTF(SPDB_INTFIX, ("... BINGO! ..."));
			
			/*
			 * OK!  we found match.  pull out the old interrupt
			 * register, patch in the new value, and put it back.
			 */
			intr = pci_conf_read(pc, tag, PCI_INTERRUPT_REG);
			DPRINTF(SPDB_INTFIX, ("\n\t    ; read %x from intreg", intr));

			intr = (intr & ~PCI_INTERRUPT_LINE_MASK) |
			       (pp->pp_intmap[i].child_intr & PCI_INTERRUPT_LINE_MASK);
			DPRINTF((SPDB_INTFIX|SPDB_INTMAP), ("\n\t    ; gonna write %x to intreg", intr));
			pci_conf_write(pc, tag, PCI_INTERRUPT_REG, intr);
			DPRINTF((SPDB_INTFIX|SPDB_INTMAP), ("\n\t    ; reread %x from intreg", intr));
			break;
		}

		/* enable mem & dma if not already */
		pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG,
			PCI_COMMAND_MEM_ENABLE|PCI_COMMAND_MASTER_ENABLE);


		/* clean up */
		if (pr)
			free(pr, M_DEVBUF);
clean2:
		if (ap)
			free(ap, M_DEVBUF);
clean1:
		if (ip)
			free(ip, M_DEVBUF);
	}
	DPRINTF(SPDB_INTFIX, ("\n"));
}

int
pci_bus_maxdevs(pc, busno)
	pci_chipset_tag_t pc;
	int busno;
{

	return 32;
}

pcitag_t
pci_make_tag(pc, b, d, f)
	pci_chipset_tag_t pc;
	int b;
	int d;
	int f;
{

	/* make me a useable offset */
	return (b << 16) | (d << 11) | (f << 8);
}

static int confaddr_ok __P((struct psycho_softc *, pcitag_t));

/*
 * this function is a large hack.  ideally, we should also trap accesses
 * properly, but we have to avoid letting anything read various parts
 * of bus 0 dev 0 fn 0 space or the machine may hang.  so, even if we
 * do properly implement PCI config access trap handling, this function
 * should remain in place Just In Case.
 */
static int
confaddr_ok(sc, tag)
	struct psycho_softc *sc;
	pcitag_t tag;
{
	int bus, dev, fn;

	bus = TAG2BUS(tag);
	dev = TAG2DEV(tag);
	fn = TAG2FN(tag);

	if (sc->sc_mode == PSYCHO_MODE_SABRE) {
		/*
		 * bus 0 is only ok for dev 0 fn 0, dev 1 fn 0 and dev fn 1.
		 */
		if (bus == 0 && 
		    ((dev == 0 && fn > 0) ||
		     (dev == 1 && fn > 1) ||
		     (dev > 1))) {
			DPRINTF(SPDB_CONF, (" confaddr_ok: rejecting bus %d dev %d fn %d -", bus, dev, fn));
			return (0);
		}
	} else if (sc->sc_mode == PSYCHO_MODE_PSYCHO_A ||
		   sc->sc_mode == PSYCHO_MODE_PSYCHO_B) {
		/*
		 * make sure we are reading our own bus
		 */
		/* XXX??? */
		panic("confaddr_ok: can't do SUNW,psycho yet");
	}
	return (1);
}

/* assume we are mapped little-endian/side-effect */
pcireg_t
pci_conf_read(pc, tag, reg)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int reg;
{
	struct psycho_pbm *pp = pc->cookie;
	struct psycho_softc *sc = pp->pp_sc;
	pcireg_t val;

	DPRINTF(SPDB_CONF, ("pci_conf_read: tag %lx; reg %x; ", (long)tag, reg));
	DPRINTF(SPDB_CONF, ("asi = %x; readaddr = %qx (offset = %x) ...",
		    bus_type_asi[sc->sc_configtag->type],
		    sc->sc_configaddr + tag + reg, (int)tag + reg));

	if (confaddr_ok(sc, tag) == 0) {
		val = (pcireg_t)~0;
	} else {
#if 0
		u_int32_t data;

		data = probeget(sc->sc_configaddr + tag + reg,
				bus_type_asi[sc->sc_configtag->type], 4);
		if (data == -1)
			val = (pcireg_t)~0;
		else
			val = (pcireg_t)data;
#else
		membar_sync();
		val = bus_space_read_4(sc->sc_configtag, sc->sc_configaddr,
		    tag + reg);
		membar_sync();
#endif
	}
	DPRINTF(SPDB_CONF, (" returning %08x\n", (u_int)val));

	return (val);
}

void
pci_conf_write(pc, tag, reg, data)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int reg;
	pcireg_t data;
{
	struct psycho_pbm *pp = pc->cookie;
	struct psycho_softc *sc = pp->pp_sc;

	DPRINTF(SPDB_CONF, ("pci_conf_write: tag %ld; reg %d; data %d; ", (long)tag, reg, (int)data));
	DPRINTF(SPDB_CONF, ("asi = %x; readaddr = %qx (offset = %x)\n",
		    bus_type_asi[sc->sc_configtag->type],
		    sc->sc_configaddr + tag + reg, (int)tag + reg));

	if (confaddr_ok(sc, tag) == 0)
		panic("pci_conf_write: bad addr");
		
#if 0
	probeset(sc->sc_configaddr + tag + reg,
		 bus_type_asi[sc->sc_configtag->type],
		 4, data);
#else
	membar_sync();
	bus_space_write_4(sc->sc_configtag, sc->sc_configaddr, tag + reg, data);
	membar_sync();
#endif
}

/*
 * interrupt mapping foo.
 */
int
pci_intr_map(pc, tag, pin, line, ihp)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int pin;
	int line;
	pci_intr_handle_t *ihp;
{
	int rv;

	/*
	 * XXX
	 * UltraSPARC IIi PCI does not use PCI_INTERRUPT_REG, but we have
	 * used this space for our own purposes...
	 */
	DPRINTF(SPDB_INTR, ("pci_intr_map: tag %lx; pin %d; line %d", 
		(long)tag, pin, line));
#if 1
	if (line == 255) {
		*ihp = -1;
		rv = 1;
		goto out;
	}
#endif
	if (pin > 4)
		panic("pci_intr_map: pin > 4");

	rv = psycho_intr_map(tag, pin, line, ihp);

out:
	DPRINTF(SPDB_INTR, ("; handle = %d; returning %d\n", (int)*ihp, rv));
	return (rv);
}

const char *
pci_intr_string(pc, ih)
	pci_chipset_tag_t pc;
	pci_intr_handle_t ih;
{
	static char str[16];

	DPRINTF(SPDB_INTR, ("pci_intr_string: ih %u", ih));
	if (ih < 0 || ih > 0x32) {
		printf("\n");	/* i'm *so* beautiful */
		panic("pci_intr_string: bogus handle\n");
	}
	sprintf(str, "vector %u", ih);
	DPRINTF(SPDB_INTR, ("; returning %s\n", str));

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
	int (*func) __P((void *));
	void *arg;
{
	void *cookie;
	struct psycho_pbm *pp = (struct psycho_pbm *)pc->cookie;

	DPRINTF(SPDB_INTR, ("pci_intr_establish: ih %lu; level %d", (u_long)ih, level));
	cookie = bus_intr_establish(pp->pp_memt, ih, 0, func, arg);

	DPRINTF(SPDB_INTR, ("; returning handle %p\n", cookie));
	return (cookie);
}

void
pci_intr_disestablish(pc, cookie)
	pci_chipset_tag_t pc;
	void *cookie;
{

	DPRINTF(SPDB_INTR, ("pci_intr_disestablish: cookie %p\n", cookie));

	/* XXX */
	panic("can't disestablish PCI interrupts yet");
}
