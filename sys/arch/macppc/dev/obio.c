/*	$NetBSD: obio.c,v 1.25 2007/01/18 00:17:22 macallan Exp $	*/

/*-
 * Copyright (C) 1998	Internet Research Institute, Inc.
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
 *	This product includes software developed by
 *	Internet Research Institute, Inc.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: obio.c,v 1.25 2007/01/18 00:17:22 macallan Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/ofw/openfirm.h>

#include <machine/autoconf.h>

#include <sys/sysctl.h>

static void obio_attach __P((struct device *, struct device *, void *));
static int obio_match __P((struct device *, struct cfdata *, void *));
static int obio_print __P((void *, const char *));

struct obio_softc {
	struct device sc_dev;
	int sc_node;
};

static void obio_setup_ohare2(struct obio_softc *, struct confargs *);
static int obio_get_cpu_speed(paddr_t);
static void obio_set_cpu_speed(paddr_t, int);
static void setup_sysctl(paddr_t);

CFATTACH_DECL(obio, sizeof(struct obio_softc),
    obio_match, obio_attach, NULL, NULL);

int
obio_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_APPLE)
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_APPLE_GC:
		case PCI_PRODUCT_APPLE_OHARE:
		case PCI_PRODUCT_APPLE_HEATHROW:
		case PCI_PRODUCT_APPLE_PADDINGTON:
		case PCI_PRODUCT_APPLE_KEYLARGO:
		case PCI_PRODUCT_APPLE_PANGEA_MACIO:
		case PCI_PRODUCT_APPLE_INTREPID:
		case PCI_PRODUCT_APPLE_K2:
			return 1;
		}

	return 0;
}

/*
 * Attach all the sub-devices we can find
 */
void
obio_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct obio_softc *sc = (struct obio_softc *)self;
	struct pci_attach_args *pa = aux;
	struct confargs ca;
	int node, child, namelen;
	u_int reg[20];
	int intr[6], parent_intr = 0, parent_nintr = 0;
	char name[32];
	char compat[32];

	switch (PCI_PRODUCT(pa->pa_id)) {

	case PCI_PRODUCT_APPLE_GC:
	case PCI_PRODUCT_APPLE_OHARE:
	case PCI_PRODUCT_APPLE_HEATHROW:
	case PCI_PRODUCT_APPLE_PADDINGTON:
	case PCI_PRODUCT_APPLE_KEYLARGO:
	case PCI_PRODUCT_APPLE_PANGEA_MACIO:
	case PCI_PRODUCT_APPLE_INTREPID:
		node = pcidev_to_ofdev(pa->pa_pc, pa->pa_tag);
		if (node == -1)
			node = OF_finddevice("mac-io");
			if (node == -1)
				node = OF_finddevice("/pci/mac-io");
		break;
	case PCI_PRODUCT_APPLE_K2:
		node = OF_finddevice("mac-io");
		if (node == -1)
			panic("macio not found");
		break;

	default:
		printf("obio_attach: unknown obio controller\n");
		return;
	}

	sc->sc_node = node;

#if defined (PMAC_G5)
	if (OF_getprop(node, "assigned-addresses", reg, sizeof(reg)) < 20)
	{
		return;
	}
#else
	if (OF_getprop(node, "assigned-addresses", reg, sizeof(reg)) < 12)
		return;
#endif /* PMAC_G5 */

	ca.ca_baseaddr = reg[2];

	printf(": addr 0x%x\n", ca.ca_baseaddr);

	/* Enable internal modem (KeyLargo) */
	if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_KEYLARGO) {
		printf("enabling KeyLargo internal modem\n");
		out32rb(ca.ca_baseaddr + 0x40,
			in32rb(ca.ca_baseaddr + 0x40) & ~((u_int32_t)1<<25));
	}
	
	if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_INTREPID) {
		paddr_t addr;
		
		printf("enabling Intrepid CPU speed control\nCPU speed is ");
		addr = ca.ca_baseaddr + 0x6a;
		if (obio_get_cpu_speed(addr)) {
			printf("high\n");
		} else {
			printf("low\n");
		}
		setup_sysctl(addr);
	}
	
	/* Enable internal modem (Pangea) */
	if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_PANGEA_MACIO) {
		out8(ca.ca_baseaddr + 0x006a + 0x03, 0x04); /* set reset */
		out8(ca.ca_baseaddr + 0x006a + 0x02, 0x04); /* power modem on */
		out8(ca.ca_baseaddr + 0x006a + 0x03, 0x05); /* unset reset */ 
	}

	/* Gatwick and Paddington use same product ID */
	namelen = OF_getprop(node, "compatible", compat, sizeof(compat));

	if (strcmp(compat, "gatwick") == 0) {
		parent_nintr = OF_getprop(node, "AAPL,interrupts", intr,
					sizeof(intr));
		parent_intr = intr[0];
	} else {
  		/* Enable CD and microphone sound input. */
		if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_PADDINGTON)
			out8(ca.ca_baseaddr + 0x37, 0x03);
	}
	
	if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_OHARE) {
		uint32_t freg;
		
		freg = in32rb(ca.ca_baseaddr + 0x38);
		printf("%s: FCR %08x\n", sc->sc_dev.dv_xname, freg);

		if (ca.ca_baseaddr != 0xf3000000) {
			obio_setup_ohare2(sc, &ca);
			return;
		}
		printf("enabling 2nd IDE channel on ohare\n");
		freg |= 8;
		out32rb(ca.ca_baseaddr + 0x38, freg);
	}

	for (child = OF_child(node); child; child = OF_peer(child)) {
		namelen = OF_getprop(child, "name", name, sizeof(name));
		if (namelen < 0)
			continue;
		if (namelen >= sizeof(name))
			continue;

		name[namelen] = 0;
		ca.ca_name = name;
		ca.ca_node = child;
		ca.ca_tag = pa->pa_memt;
		
		ca.ca_nreg = OF_getprop(child, "reg", reg, sizeof(reg));
		if (strcmp(name, "backlight") == 0) {
			paddr_t addr = (ca.ca_baseaddr + reg[0]);
			printf("backlight: %08x %08x\n",(uint32_t)addr, in32rb(addr));
		}
		if (strcmp(compat, "gatwick") != 0) {
			ca.ca_nintr = OF_getprop(child, "AAPL,interrupts", intr,
					sizeof(intr));
			if (ca.ca_nintr == -1)
				ca.ca_nintr = OF_getprop(child, "interrupts", intr,
						sizeof(intr));
		} else {
			intr[0] = parent_intr;
			ca.ca_nintr = parent_nintr;
		}
		ca.ca_reg = reg;
		ca.ca_intr = intr;

		config_found(self, &ca, obio_print);
	}
}

static const char *skiplist[] = {
	"interrupt-controller",
	"gpio",
	"escc-legacy",
	"timer",
	"i2c",
	"power-mgt",
	"escc"
	
};

#define N_LIST (sizeof(skiplist) / sizeof(skiplist[0]))

int
obio_print(aux, obio)
	void *aux;
	const char *obio;
{
	struct confargs *ca = aux;
	int i;

	for (i = 0; i < N_LIST; i++)
		if (strcmp(ca->ca_name, skiplist[i]) == 0)
			return QUIET;

	if (obio)
		aprint_normal("%s at %s", ca->ca_name, obio);

	if (ca->ca_nreg > 0)
		aprint_normal(" offset 0x%x", ca->ca_reg[0]);

	return UNCONF;
}

static void
obio_setup_ohare2(struct obio_softc *sc, struct confargs *ca)
{
	printf("ohare2: %08x\n", ca->ca_baseaddr);
}

static void
obio_set_cpu_speed(paddr_t addr, int fast)
{

	if(addr != 0) {
		if (fast) {
			out8rb(addr + 1, 5);	/* bump Vcore */
			out8rb(addr, 5);	/* bump CPU speed */
		} else {
			out8rb(addr, 4);	/* lower CPU speed */
			out8rb(addr + 1, 4);	/* lower Vcore */
		}
	}
}

static int
obio_get_cpu_speed(paddr_t addr)
{
	
	if(addr != 0) {
		if(in8rb(addr) & 1)
			return 1;
	}
	return 0;
}

SYSCTL_SETUP(sysctl_cpuspeed_setup, "sysctl cpu speed setup")
{

	sysctl_createv(NULL, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "machdep", NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CTL_EOL);
}

static int
sysctl_cpuspeed_temp(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	paddr_t addr = (paddr_t)node.sysctl_data;
	const int *np = newp;
	int speed, nd = 0;

	speed = obio_get_cpu_speed(addr);	
	node.sysctl_idata = speed;
	if (np) {
		/* we're asked to write */	
		nd = *np;
		node.sysctl_data = &speed;
		if (sysctl_lookup(SYSCTLFN_CALL(&node)) == 0) {
			int new_reg;
			
			new_reg = (max(0, min(1, node.sysctl_idata)));
			obio_set_cpu_speed(addr, new_reg);
			return 0;
		}
		return EINVAL;
	} else {
		node.sysctl_size = 4;
		return(sysctl_lookup(SYSCTLFN_CALL(&node)));
	}
}

static void
setup_sysctl(paddr_t addr)
{
	struct sysctlnode *node=NULL;
	int ret;
	
	ret = sysctl_createv(NULL, 0, NULL, 
	    (const struct sysctlnode **)&node, 
	    CTLFLAG_READWRITE | CTLFLAG_OWNDESC | CTLFLAG_IMMEDIATE,
	    CTLTYPE_INT, "cpu_speed", "CPU speed", sysctl_cpuspeed_temp, 
		    addr , NULL, 0, CTL_MACHDEP, CTL_CREATE, CTL_EOL);
	if (node != NULL) {
		node->sysctl_data = (void *)addr;
	}
}
