/* $Id: */

/*
 * Derived from  pci.c
 * Copyright (c) 2000 Geocast Networks Systems.  All rights reserved.
 *
 * Copyright (c) 1995, 1996, 1997, 1998
 *     Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1994 Charles M. Hannum.  All rights reserved.
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
 *	This product includes software developed by Charles M. Hannum.
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
 * Machine independent support for PCI serial console support.
 *
 * Scan the PCI bus for something which resembles a 16550
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <sys/termios.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <dev/cons.h>

#include <dev/pci/pucvar.h>
#include <dev/pci/puccn.h>

#ifndef CONSPEED
#define CONSPEED	TTYDEF_SPEED
#endif
#ifndef CONMODE
#define	CONMODE		((TTYDEF_CFLAG & ~(CSIZE|CSTOPB|PARENB))|CS8) /* 8N1 */
#endif

#ifdef i386		/* Handle i386 directly */
int
cpu_comcnprobe(struct consdev *cn, struct pci_attach_args *pa)
{
	pci_mode_detect();
	pa->pa_iot = I386_BUS_SPACE_IO;
	pa->pa_pc = 0;
	pa->pa_tag = pci_make_tag(0, 0, 31, 0);
	return 0;
}
#endif

cons_decl(com);

static bus_addr_t puccnbase;
static bus_space_tag_t puctag;

#ifdef KGDB
static bus_addr_t pucgdbbase;
#endif

/*
 * Static dev/func variables allow pucprobe to be called multiple times,
 * resuming the search where it left off, never retrying the same adaptor.
 */

static bus_addr_t
pucprobe_doit(struct consdev *cn)
{
	struct pci_attach_args pa;
	int bus;
	static int dev = 0, func = 0;
	int maxdev, nfunctions, i;
	pcireg_t reg, bhlcr, subsys;
	int foundport = 0;
	const struct puc_device_description *desc;
	pcireg_t base;

	/* Fetch our tags */
	if (cpu_comcnprobe(cn, &pa) != 0) {
		return 0;
	}
	puctag = pa.pa_iot;
	pci_decompose_tag(pa.pa_pc, pa.pa_tag, &bus, &maxdev, NULL);

	/* scan through devices */

	for (; dev <= maxdev ; dev++) {
		pa.pa_tag = pci_make_tag(pa.pa_pc, bus, dev, 0);
		reg = pci_conf_read(pa.pa_pc, pa.pa_tag, PCI_ID_REG);
		if (PCI_VENDOR(reg) == PCI_VENDOR_INVALID
		    || PCI_VENDOR(reg) == 0)
			continue;
		bhlcr = pci_conf_read(pa.pa_pc, pa.pa_tag, PCI_BHLC_REG);
		if (PCI_HDRTYPE_MULTIFN(bhlcr)) {
			nfunctions = 8;
		} else {
			nfunctions = 1;
		}
resume_scan:
		for (; func < nfunctions; func++)  {
			pa.pa_tag = pci_make_tag(pa.pa_pc, bus, dev, func);
			reg = pci_conf_read(pa.pa_pc, pa.pa_tag, PCI_CLASS_REG);
			if (PCI_CLASS(reg)  == PCI_CLASS_COMMUNICATIONS
			    && PCI_SUBCLASS(reg)
			       == PCI_SUBCLASS_COMMUNICATIONS_SERIAL) {
				pa.pa_id = pci_conf_read(pa.pa_pc, pa.pa_tag, PCI_ID_REG);
				subsys = pci_conf_read(pa.pa_pc, pa.pa_tag,
				    PCI_SUBSYS_ID_REG);
				foundport = 1;
				break;
			}
		}
		if (foundport)
			break;

		func = 0;
	}
	if (!foundport)
		return 0;
	foundport = 0;

	desc = puc_find_description(PCI_VENDOR(pa.pa_id),
	    PCI_PRODUCT(pa.pa_id), PCI_VENDOR(subsys), PCI_PRODUCT(subsys));
	if (desc == NULL) {
		func++;
		goto resume_scan;
	}

	for (i = 0; PUC_PORT_VALID(desc, i); i++)
	{
		if (desc->ports[i].type != PUC_PORT_TYPE_COM)
			continue;
		base = pci_conf_read(pa.pa_pc, pa.pa_tag, desc->ports[i].bar);
		base += desc->ports[i].offset;

		if (PCI_MAPREG_TYPE(base) != PCI_MAPREG_TYPE_IO)
			continue;
		base = PCI_MAPREG_IO_ADDR(base);
		if (com_is_console(puctag, base, NULL))
			continue;
		foundport = 1;
		break;
	}
		
	if (foundport == 0) {
		func++;
		goto resume_scan;
	}

	cn->cn_pri = CN_REMOTE;
	return PCI_MAPREG_IO_ADDR(base);
}

#ifdef KGDB
void comgdbprobe(struct consdev *);
void comgdbinit(struct consdev *);

void
comgdbprobe(struct consdev *cn)
{
	pucgdbbase = pucprobe_doit(cn);
}

void
comgdbinit(struct consdev *cn)
{
	if (pucgdbbase == 0) {
		return;
	}
	com_kgdb_attach(puctag, pucgdbbase, CONSPEED, COM_FREQ, CONMODE);
}
#endif

void
comcnprobe(struct consdev *cn)
{
	puccnbase = pucprobe_doit(cn);
}

void
comcninit(struct consdev *cn)
{
	if (puccnbase == 0) {
		return;
	}
	comcnattach(puctag, puccnbase, CONSPEED, COM_FREQ, CONMODE);
}

/* comcngetc, comcnputc, comcnpollc provided by dev/ic/com.c */
