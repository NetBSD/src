/*	$NetBSD: if_le_oioc.c,v 1.1.6.2 2009/05/13 17:18:18 jym Exp $	*/

/*
 * Copyright (c) 2009 Stephen M. Rumble
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_le_oioc.c,v 1.1.6.2 2009/05/13 17:18:18 jym Exp $");

#include "opt_inet.h"
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/device.h>

#include <uvm/uvm.h>	// for uvm_pglistalloc

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#include <machine/cpu.h>
#include <machine/machtype.h>

#include <dev/ic/lancereg.h>
#include <dev/ic/lancevar.h>
#include <dev/ic/am7990reg.h>
#include <dev/ic/am7990var.h>

#include <dev/arcbios/arcbios.h>
#include <dev/arcbios/arcbiosvar.h>

#include <sgimips/ioc/oiocvar.h>
#include <sgimips/ioc/oiocreg.h>

#include <mips/include/cache.h>

#ifndef OIOC_LANCE_NPAGES
#define OIOC_LANCE_NPAGES	64	/* 256KB */
#endif

#if OIOC_LANCE_NPAGES > OIOC_ENET_NPGMAPS
#error OIOC_LANCE_NPAGES > OIOC_ENET_NPGMAPS (512)
#endif

/*
 * Ethernet software status per interface.
 * The real stuff is in dev/ic/am7990var.h
 * The really real stuff is in dev/ic/lancevar.h
 *
 * Lance is somewhat nasty MI code. We basically get:
 *	struct le_softc {
 *		struct am7990_softc {
 *			struct lance_softc {
 *				device_t sc_dev;
 *				...
 *			}
 * 		}
 *
 *		bus_space_tag ...
 *	}
 *
 * So, we can cast any three to any other three, plus sc_dev->dv_private points
 * back at the top (i.e. to le_softc, am7990_softc and lance_softc). Bloody
 * hell!
 */
struct le_softc {
	struct	am7990_softc sc_am7990;		/* glue to MI code */

	bus_space_tag_t      sc_st;
	bus_space_handle_t   sc_maph;		/* ioc<->lance page map regs */
	bus_space_handle_t   sc_rdph;		/* lance rdp */
	bus_space_handle_t   sc_raph;		/* lance rap */
};

static int	le_match(device_t, cfdata_t, void *);
static void	le_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(le, sizeof(struct le_softc),
    le_match, le_attach, NULL, NULL);

#if defined(_KERNEL_OPT)
#include "opt_ddb.h"
#endif

static void	lewrcsr(struct lance_softc *, uint16_t, uint16_t);
static uint16_t	lerdcsr(struct lance_softc *, uint16_t);  
static void	enaddr_aton(const char *, u_int8_t *);

static void
lewrcsr(struct lance_softc *sc, uint16_t port, uint16_t val)
{
	struct le_softc *lesc = (struct le_softc *)sc;
	bus_space_write_2(lesc->sc_st, lesc->sc_raph, 0, port);
	bus_space_write_2(lesc->sc_st, lesc->sc_rdph, 0, val);
}

static uint16_t
lerdcsr(struct lance_softc *sc, uint16_t port)
{
	struct le_softc *lesc = (struct le_softc *)sc;
	bus_space_write_2(lesc->sc_st, lesc->sc_raph, 0, port);
	return (bus_space_read_2(lesc->sc_st, lesc->sc_rdph, 0));
} 

/*
 * Always present on IP6 and IP10. IP4? Unknown.
 */
int
le_match(device_t parent, cfdata_t cf, void *aux)
{
	struct oioc_attach_args *oa = aux;

	if (mach_type == MACH_SGI_IP4)
		return (0);

        if (strcmp(oa->oa_name, cf->cf_name) == 0)
		return (1);

	return (0);
}

void
le_attach(device_t parent, device_t self, void *aux)
{
	extern paddr_t avail_start, avail_end;

	struct le_softc *lesc = device_private(self);
	struct lance_softc *sc = &lesc->sc_am7990.lsc;
	struct oioc_attach_args *oa = aux;
	struct pglist mlist;
	const char *enaddrstr;
	char enaddr[6];
	char pbuf[9];
	int i, error;

	sc->sc_dev = self;
	lesc->sc_st = oa->oa_st;

	enaddrstr = ARCBIOS->GetEnvironmentVariable("eaddr");
	if (enaddrstr == NULL) {
		aprint_error(": failed to obtain MAC address\n");
		return;
	}

        if ((error = bus_space_subregion(oa->oa_st, oa->oa_sh, OIOC_LANCE_RDP,
	    OIOC_LANCE_RDP_SIZE, &lesc->sc_rdph)) != 0) {
		printf(": unable to map rdp reg, error=%d\n", error);
		goto fail_0;
	}

        if ((error = bus_space_subregion(oa->oa_st, oa->oa_sh, OIOC_LANCE_RAP,
	    OIOC_LANCE_RAP_SIZE, &lesc->sc_raph)) != 0) {
		printf(": unable to map rap reg, error=%d\n", error);
		goto fail_1;
	}

        if ((error = bus_space_subregion(oa->oa_st, oa->oa_sh,
	    OIOC_ENET_PGMAP_BASE, OIOC_ENET_PGMAP_SIZE, &lesc->sc_maph)) != 0) {
		printf(": unable to map rap reg, error=%d\n", error);
		goto fail_2;
	}

	/* Allocate a contiguous chunk of physical memory for the le buffer. */
	error = uvm_pglistalloc(OIOC_LANCE_NPAGES * PAGE_SIZE,
	    avail_start, avail_end, PAGE_SIZE, 0, &mlist, 1, 0);
	if (error) {
		aprint_error(": failed to allocate ioc<->lance buffer space, "
		    "error = %d\n", error);
		goto fail_3;
	}

	/* Use IOC to map the physical memory into the Ethernet chip's space. */
	for (i = 0; i < OIOC_LANCE_NPAGES; i++) {
		bus_space_write_2(lesc->sc_st,lesc->sc_maph,
		    OIOC_ENET_PGMAP_OFF(i),
		    (VM_PAGE_TO_PHYS(mlist.tqh_first) >> PAGE_SHIFT) + i);
	}

	sc->sc_mem = (void *)MIPS_PHYS_TO_KSEG1(
	    (uint32_t)VM_PAGE_TO_PHYS(mlist.tqh_first));
	sc->sc_memsize = OIOC_LANCE_NPAGES * PAGE_SIZE;
	sc->sc_addr = 0;
	sc->sc_conf3 = LE_C3_BSWP;

	enaddr_aton(enaddrstr, enaddr);
	memcpy(sc->sc_enaddr, enaddr, sizeof(sc->sc_enaddr));

	if (cpu_intr_establish(oa->oa_irq, IPL_NET, am7990_intr, sc) == NULL) {
		aprint_error(": failed to establish interrupt %d\n",oa->oa_irq);
		goto fail_4;
	}

	sc->sc_copytodesc   = lance_copytobuf_contig;
	sc->sc_copyfromdesc = lance_copyfrombuf_contig;
	sc->sc_copytobuf    = lance_copytobuf_contig;
	sc->sc_copyfrombuf  = lance_copyfrombuf_contig;
	sc->sc_zerobuf      = lance_zerobuf_contig;

	sc->sc_rdcsr  = lerdcsr;
	sc->sc_wrcsr  = lewrcsr;
	sc->sc_hwinit = NULL;

	format_bytes(pbuf, sizeof(pbuf), OIOC_LANCE_NPAGES * PAGE_SIZE);
	aprint_normal(": main memory used = %s\n", pbuf);
	aprint_normal("%s", device_xname(self));

	am7990_config(&lesc->sc_am7990);

	return;

fail_4:
	uvm_pglistfree(&mlist);
fail_3:
	bus_space_unmap(oa->oa_st, oa->oa_sh, lesc->sc_maph);
fail_2:
	bus_space_unmap(oa->oa_st, oa->oa_sh, lesc->sc_raph);
fail_1:
	bus_space_unmap(oa->oa_st, oa->oa_sh, lesc->sc_rdph);
fail_0:
	return;
}

/* stolen from sgimips/hpc/if_sq.c */
static void
enaddr_aton(const char *str, u_int8_t *eaddr)
{
	int i;
	char c;

	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		if (*str == ':')
			str++;

		c = *str++;
		if (isdigit(c)) {
			eaddr[i] = (c - '0');
		} else if (isxdigit(c)) {
			eaddr[i] = (toupper(c) + 10 - 'A');
		}

		c = *str++;
		if (isdigit(c)) {
			eaddr[i] = (eaddr[i] << 4) | (c - '0');
		} else if (isxdigit(c)) {
			eaddr[i] = (eaddr[i] << 4) | (toupper(c) + 10 - 'A');
		}
	}
}
