/*	$NetBSD: consinit.c,v 1.1 2009/07/20 04:41:36 kiyohara Exp $	*/
/*
 * Copyright (c) 2009 KIYOHARA Takashi
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: consinit.c,v 1.1 2009/07/20 04:41:36 kiyohara Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/termios.h>

#include <machine/bus.h>
#include <machine/dig64.h>
#include <machine/md_var.h>

#include <dev/acpi/acpica.h>

#include "com.h"
#include "vga.h"
#include "ssccons.h"

#include <dev/cons.h>
#if NCOM > 0
#include <dev/ic/comvar.h>
#endif
#if NVGA > 0
#include <dev/ic/vgareg.h>
#include <dev/ic/vgavar.h>
#endif


static void pcdp_cnprobe(struct consdev *);
static void pcdp_cninit(struct consdev *);

cons_decl(ssc);

struct consdev constab[] = {
	{ pcdp_cnprobe, pcdp_cninit,
	  NULL, NULL, NULL, NULL, NULL, NULL, NODEV, CN_DEAD },
#if NSSCCONS > 0
	cons_init(ssc),
#endif
	{ NULL }
};


void
consinit()
{

	cninit();
}

static void
pcdp_cnprobe(struct consdev *cn)
{
	struct dig64_hcdp_table *tbl;
	union dev_desc *desc;
	uint64_t hcdp;
	int n, m;

	hcdp = ia64_get_hcdp();
	if (hcdp != 0) {
		tbl = (void*)IA64_PHYS_TO_RR7(hcdp);
		n = 0;
		m = tbl->length - sizeof(struct dig64_hcdp_table);
		while (n < m) {
			desc = (union dev_desc *)((char *)tbl->entry + n);
#if NVGA > 0
			if (devdesc->type ==
			    (DIG64_ENTRYTYPE_VGA | DIG64_ENTRYTYPE_OUTONLY)) {
#if defined(DIAGNOSTIC)
				if (tbl->revision < 3)
					panic("PCDP found in HCDP rev.%d."
					    " Maybe unsupport PCDP",
					    tbl->revision);
#endif
				cn->cn_pri = CN_NORMAL;
				break;
			}
#endif
#if NCOM > 0
			if (desc->type == DIG64_HCDP_CONSOLE) {
				cn->cn_pri = CN_REMOTE;
				break;
			}
#endif

			if (desc->type == DIG64_ENTRYTYPE_TYPE0 ||
			    desc->type == DIG64_ENTRYTYPE_TYPE1)
				n += sizeof(struct dig64_hcdp_entry);
			else
				n += desc->pcdp.length;
		}
	}
	if (cn->cn_pri != CN_DEAD)
		cn->cn_dev = ~NODEV;	/* Shall we makedev()? */
}

static void
pcdp_cninit(struct consdev *cn)
{
	struct dig64_hcdp_table *tbl;
	union dev_desc *desc;
	uint64_t hcdp;
	int n, m;

	hcdp = ia64_get_hcdp();
	if (hcdp == 0)
		panic("lost console...\n");

	tbl = (void *)IA64_PHYS_TO_RR7(hcdp);
	n = 0;
	m = tbl->length - sizeof(struct dig64_hcdp_table);
	while (n < m) {
		desc = (union dev_desc *)((char *)tbl->entry + n);
#if NVGA > 0

/* not yet... */
/* Our VGA is Framebuffer? */

		if (cn->cn_pri == CN_NORMAL &&
		    desc->type ==
			    (DIG64_ENTRYTYPE_VGA | DIG64_ENTRYTYPE_OUTONLY)) {
			struct dig64_pcdp_entry *ent = &desc->pcdp;

			if (ent->specs.type == DIG64_PCDP_SPEC_PCI) {
				struct dig64_pci_spec *spec = ent->specs.pci;

				if (spec->flags & DIG64_FLAGS_MMIO_TRA_VALID)
					(void*)spec->mmio_tra;
				if (spec->flags & DIG64_FLAGS_IOPORT_TRA_VALID)
					(void*)spec->ioport_tra;
			}

			break;
		}
#endif
#if NCOM > 0
		if (cn->cn_pri == CN_REMOTE &&
		    desc->type == DIG64_HCDP_CONSOLE) {
			struct dig64_hcdp_entry *ent = &desc->uart;
			bus_addr_t ioaddr;
			bus_space_tag_t iot;
			const uint64_t rate =
			    ((uint64_t)ent->baud_high << 32) | ent->baud_low;
			tcflag_t cflag =
			    TTYDEF_CFLAG & ~(CSIZE | PARENB | PARODD);

			switch (ent->databits) {
			case 5:
				cflag = CS5;
				break;
			case 6:
				cflag = CS6;
				break;
			case 7:
				cflag = CS7;
				break;
			case 8:
				cflag = CS8;
				break;
			default:
				panic("unsupported databits %d\n",
				    ent->databits);
			}
			switch (ent->parity) {
			case DIG64_HCDP_PARITY_NO:
				break;
			case DIG64_HCDP_PARITY_ODD:
				cflag |= PARODD;

				/* FALLTHROUGH */

			case DIG64_HCDP_PARITY_EVEN:
				cflag |= PARENB;
				break;
			case DIG64_HCDP_PARITY_MARK:
			case DIG64_HCDP_PARITY_SPACE:
			default:
				panic("unsupported parity type %d\n",
				    ent->parity);
			}
			if (ent->stopbits == DIG64_HCDP_STOPBITS_1)
				cflag &= ~CSTOPB;
			else if (ent->stopbits == DIG64_HCDP_STOPBITS_2)
				cflag |= CSTOPB;
			else
				panic("unsupported stopbits type %d\n",
				    ent->stopbits);
			iot = (ent->address.addr_space ==
						ACPI_ADR_SPACE_SYSTEM_MEMORY) ?
			    IA64_BUS_SPACE_MEM : IA64_BUS_SPACE_IO;
			ioaddr = ((uint64_t)ent->address.addr_high << 32) |
			    ent->address.addr_low;
			comcnattach(iot, ioaddr, rate,
			    (ent->pclock != 0) ? ent->pclock : COM_FREQ,
			    COM_TYPE_NORMAL, cflag);
			break;
		}
#endif
	}
}
