/*	$NetBSD: if_ae_nubus.c,v 1.2 1997/02/24 07:34:19 scottr Exp $	*/

/*
 * Copyright (C) 1997 Scott Reynolds
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
 *      This product includes software developed by Scott Reynolds for
 *      the NetBSD Project.
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

#include "bpfilter.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/systm.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/netisr.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include <machine/bus.h>
#include <machine/viareg.h>

#include "nubus.h"
#include <dev/ic/dp8390reg.h>
#include "if_aereg.h"
#include "if_aevar.h"

static int	ae_nubus_match __P((struct device *, struct cfdata *, void *));
static void	ae_nubus_attach __P((struct device *, struct device *, void *));
static int	ae_card_vendor __P((struct nubus_attach_args *na));

struct cfattach ae_nubus_ca = {
	sizeof(struct ae_softc), ae_nubus_match, ae_nubus_attach
};

static int
ae_nubus_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct nubus_attach_args *na = (struct nubus_attach_args *) aux;
	bus_space_handle_t bsh;
	int rv;

	if (bus_space_map(na->na_tag, NUBUS_SLOT2PA(na->slot), NBMEMSIZE,
	    0, &bsh))
		return (0);

	rv = 0;

	if (na->category == NUBUS_CATEGORY_NETWORK &&
	    na->type == NUBUS_TYPE_ETHERNET) {
		switch (ae_card_vendor(na)) {
		case AE_VENDOR_APPLE:
		case AE_VENDOR_ASANTE:
		case AE_VENDOR_FARALLON:
		case AE_VENDOR_INTERLAN:
		case AE_VENDOR_KINETICS:
			rv = 1;
			break;
		case AE_VENDOR_DAYNA:
		case AE_VENDOR_FOCUS:
			rv = UNSUPP;
			break;
		default:
			break;
		}
	}

	bus_space_unmap(na->na_tag, bsh, NBMEMSIZE);

	return rv;
}

/*
 * Install interface into kernel networking data structures
 */
static void
ae_nubus_attach(parent, self, aux)
	struct device *parent, *self;
	void   *aux;
{
	struct ae_softc *sc = (struct ae_softc *) self;
	struct nubus_attach_args *na = (struct nubus_attach_args *) aux;
	struct ifnet *ifp = &sc->sc_arpcom.ac_if;
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	int i, success;
	int flags = 0;

	bst = na->na_tag;
	if (bus_space_map(bst, NUBUS_SLOT2PA(na->slot), NBMEMSIZE,
	    0, &bsh)) {
		printf(": can't map memory space\n");
		return;
	}

	sc->regs_rev = 0;
	sc->use16bit = 1;
	sc->vendor = ae_card_vendor(na);
	strncpy(sc->type_str, nubus_get_card_name(na->fmt),
	    INTERFACE_NAME_LEN);
	sc->type_str[INTERFACE_NAME_LEN-1] = '\0';
	sc->mem_size = 0;

	success = 0;

	switch (sc->vendor) {
	case AE_VENDOR_INTERLAN:
		if (bus_space_subregion(bst, bsh,
		    GC_REG_OFFSET, AE_REG_SIZE, &sc->sc_reg_handle)) {
			printf(": failed to map register space\n");
			break;
		}
		if ((sc->mem_size = ae_size_card_memory(bst, bsh,
		    GC_DATA_OFFSET)) == 0) {
			printf(": failed to determine size of RAM.\n");
			break;
		}
		if (bus_space_subregion(bst, bsh,
		    GC_DATA_OFFSET, sc->mem_size, &sc->sc_buf_handle)) {
			printf(": failed to map register space\n");
			break;
		}

		/* reset the NIC chip */
		bus_space_write_1(bst, bsh, GC_RESET_OFFSET, 0);

		/* Get station address from on-board ROM */
		for (i = 0; i < ETHER_ADDR_LEN; ++i)
			sc->sc_arpcom.ac_enaddr[i] =
			    bus_space_read_1(bst, bsh, (GC_ROM_OFFSET + i * 4));

		success = 1;
		break;

		/* Apple-compatible cards */
	case AE_VENDOR_ASANTE:
	case AE_VENDOR_APPLE:
		sc->regs_rev = 1;
		if (bus_space_subregion(bst, bsh,
		    AE_REG_OFFSET, AE_REG_SIZE, &sc->sc_reg_handle)) {
			printf(": failed to map register space\n");
			break;
		}
		if ((sc->mem_size = ae_size_card_memory(bst, bsh,
		    AE_DATA_OFFSET)) == 0) {
			printf(": failed to determine size of RAM.\n");
			break;
		}
		if (bus_space_subregion(bst, bsh,
		    AE_DATA_OFFSET, sc->mem_size, &sc->sc_buf_handle)) {
			printf(": failed to map register space\n");
			break;
		}

		/* Get station address from on-board ROM */
		for (i = 0; i < ETHER_ADDR_LEN; ++i)
			sc->sc_arpcom.ac_enaddr[i] =
			    bus_space_read_1(bst, bsh, (AE_ROM_OFFSET + i * 2));

		success = 1;
		break;

	case AE_VENDOR_DAYNA:
		if (bus_space_subregion(bst, bsh,
		    DP_REG_OFFSET, AE_REG_SIZE, &sc->sc_reg_handle)) {
			printf(": failed to map register space\n");
			break;
		}
		sc->mem_size = 8192;
		if (bus_space_subregion(bst, bsh,
		    DP_DATA_OFFSET, sc->mem_size, &sc->sc_buf_handle)) {
			printf(": failed to map register space\n");
			break;
		}

		/* Get station address from on-board ROM */
		for (i = 0; i < ETHER_ADDR_LEN; ++i)
			sc->sc_arpcom.ac_enaddr[i] =
			    bus_space_read_1(bst, bsh, (DP_ROM_OFFSET + i * 2));

		printf(": unsupported Dayna hardware\n");
		break;

	case AE_VENDOR_FARALLON:
		sc->regs_rev = 1;
		if (bus_space_subregion(bst, bsh,
		    AE_REG_OFFSET, AE_REG_SIZE, &sc->sc_reg_handle)) {
			printf(": failed to map register space\n");
			break;
		}
		if ((sc->mem_size = ae_size_card_memory(bst, bsh,
		    AE_DATA_OFFSET)) == 0) {
			printf(": failed to determine size of RAM.\n");
			break;
		}
		if (bus_space_subregion(bst, bsh,
		    AE_DATA_OFFSET, sc->mem_size, &sc->sc_buf_handle)) {
			printf(": failed to map register space\n");
			break;
		}

		/* Get station address from on-board ROM */
		for (i = 0; i < ETHER_ADDR_LEN; ++i)
			sc->sc_arpcom.ac_enaddr[i] =
			    bus_space_read_1(bst, bsh, (FE_ROM_OFFSET + i));

		success = 1;
		break;

	case AE_VENDOR_FOCUS:
		printf(": unsupported Focus hardware\n");
		break;

	case AE_VENDOR_KINETICS:
		sc->use16bit = 0;
		if (bus_space_subregion(bst, bsh,
		    KE_REG_OFFSET, AE_REG_SIZE, &sc->sc_reg_handle)) {
			printf(": failed to map register space\n");
			break;
		}
		if ((sc->mem_size = ae_size_card_memory(bst, bsh,
		    KE_DATA_OFFSET)) == 0) {
			printf(": failed to determine size of RAM.\n");
			break;
		}
		if (bus_space_subregion(bst, bsh,
		    KE_DATA_OFFSET, sc->mem_size, &sc->sc_buf_handle)) {
			printf(": failed to map register space\n");
			break;
		}

		/* Get station address from on-board ROM */
#if 0
		for (i = 0; i < ETHER_ADDR_LEN; ++i)
			sc->sc_arpcom.ac_enaddr[i] =
			    bus_space_read_1(bst, bsh, (KE_ROM_OFFSET + i));
#else
		{
			nubus_dir dir;
			nubus_dirent dirent;

			/* getting the MAC address */
			nubus_get_main_dir(na->fmt, &dir);
			if (nubus_find_rsrc(na->fmt, &dir, 0x80, &dirent) <= 0)
				break;
			nubus_get_dir_from_rsrc(na->fmt, &dirent, &dir);
			if (nubus_find_rsrc(na->fmt, &dir, 0x80, &dirent) <= 0)
				break;
			if (nubus_get_ind_data(na->fmt, &dirent,
			    (caddr_t)sc->sc_arpcom.ac_enaddr,
			    ETHER_ADDR_LEN) <= 0)
				break;
		}
#endif

		success = 1;
		break;

	default:
		break;
	}

	if (!success) {
		bus_space_unmap(bst, bsh, NBMEMSIZE);
		return;
	}

	sc->sc_reg_tag = sc->sc_buf_tag = bst;

	sc->cr_proto = ED_CR_RD2;

	/* Allocate one xmit buffer if < 16k, two buffers otherwise. */
	if ((sc->mem_size < 16384) || (flags & AE_FLAGS_NO_DOUBLE_BUFFERING))
		sc->txb_cnt = 1;
	else
		sc->txb_cnt = 2;

	sc->tx_page_start = 0;
	sc->rec_page_start = sc->tx_page_start + sc->txb_cnt * ED_TXBUF_SIZE;
	sc->rec_page_stop = sc->tx_page_start + (sc->mem_size >> ED_PAGE_SHIFT);
	sc->mem_ring = sc->rec_page_start << ED_PAGE_SHIFT;

	/* Now zero memory and verify that it is clear. */
	bus_space_set_region_2(sc->sc_buf_tag, sc->sc_buf_handle, 0,
	    0, sc->mem_size / 2);

	for (i = 0; i < sc->mem_size; ++i)
		if (bus_space_read_1(sc->sc_buf_tag, sc->sc_buf_handle, i))
printf("%s: failed to clear shared memory - check configuration\n",
			    sc->sc_dev.dv_xname);

	/* Set interface to stopped condition (reset). */
	aestop(sc);

	/* Initialize ifnet structure. */
	bcopy(sc->sc_dev.dv_xname, ifp->if_xname, IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_start = aestart;
	ifp->if_ioctl = aeioctl;
	ifp->if_watchdog = aewatchdog;
	ifp->if_flags =
	    IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS | IFF_MULTICAST;

	/* Attach the interface. */
	if_attach(ifp);
	ether_ifattach(ifp);

	/* Print additional info when attached. */
	printf(": address %s, ", ether_sprintf(sc->sc_arpcom.ac_enaddr));

	printf("type %s, %dKB memory\n", sc->type_str, sc->mem_size / 1024);

#if NBPFILTER > 0
	bpfattach(&ifp->if_bpf, ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif

	/* make sure interrupts are vectored to us */
	add_nubus_intr(na->slot, aeintr, sc);

	/*
	 * XXX -- enable nubus interrupts here.  Should be done elsewhere,
	 *        but that currently breaks with some nubus video cards'
	 *	  interrupts.  So we only enable nubus interrupts if we
	 *	  have an ethernet card...  i.e., we do it here.
	 */
	enable_nubus_intr();
}

static int
ae_card_vendor(na)
	struct nubus_attach_args *na;
{
	int vendor;

	switch (na->drsw) {
	case NUBUS_DRSW_3COM:
	case NUBUS_DRSW_APPLE:
	case NUBUS_DRSW_TECHWORKS:
		vendor = AE_VENDOR_APPLE;
		break;
	case NUBUS_DRSW_ASANTE:
		vendor = AE_VENDOR_ASANTE;
		break;
	case NUBUS_DRSW_FARALLON:
		vendor = AE_VENDOR_FARALLON;
		break;
	case NUBUS_DRSW_FOCUS:
		vendor = AE_VENDOR_FOCUS;
		break;
	case NUBUS_DRSW_GATOR:
		switch (na->drhw) {
		default:
		case NUBUS_DRHW_INTERLAN:
			vendor = AE_VENDOR_INTERLAN;
			break;
		case NUBUS_DRHW_KINETICS:
			if (strncmp(
			    nubus_get_card_name(na->fmt), "EtherPort", 9) == 0)
				vendor = AE_VENDOR_KINETICS;
			else
				vendor = AE_VENDOR_DAYNA;
			break;
		}
		break;
	default:
#ifdef AE_DEBUG
		printf("Unknown ethernet drsw: %x\n", na->drsw);
#endif
		vendor = AE_VENDOR_UNKNOWN;
	}
	return vendor;
}
