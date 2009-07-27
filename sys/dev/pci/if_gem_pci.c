/*	$NetBSD: if_gem_pci.c,v 1.36 2009/07/27 18:10:53 dyoung Exp $ */

/*
 *
 * Copyright (C) 2001 Eduardo Horvath.
 * All rights reserved.
 *
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR  ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR  BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 * PCI bindings for Apple GMAC, Sun ERI and Sun GEM Ethernet controllers
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_gem_pci.c,v 1.36 2009/07/27 18:10:53 dyoung Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <machine/endian.h>

#include <uvm/uvm_extern.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#include <sys/bus.h>
#include <sys/intr.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>
#include <dev/mii/mii_bitbang.h>

#include <dev/ic/gemreg.h>
#include <dev/ic/gemvar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

/* XXX Should use Properties when that's fleshed out. */
#ifdef macppc
#include <dev/ofw/openfirm.h>
#endif /* macppc */
#ifdef __sparc__
#include <machine/promlib.h>
#endif

#ifndef GEM_USE_LOCAL_MAC_ADDRESS
#if defined (macppc) || defined (__sparc__)
#define GEM_USE_LOCAL_MAC_ADDRESS	0	/* use system-wide address */
#else
#define GEM_USE_LOCAL_MAC_ADDRESS	1
#endif
#endif


struct gem_pci_softc {
	struct	gem_softc	gsc_gem;	/* GEM device */
	void			*gsc_ih;
	pci_chipset_tag_t	gsc_pc;
	pci_intr_handle_t	gsc_handle;
};

static bool	gem_pci_estintr(struct gem_pci_softc *);
static bool	gem_pci_suspend(device_t PMF_FN_PROTO);
static bool	gem_pci_resume(device_t PMF_FN_PROTO);
static int	gem_pci_detach(device_t, int);

int	gem_pci_match(device_t, cfdata_t, void *);
void	gem_pci_attach(device_t, device_t, void *);

CFATTACH_DECL3_NEW(gem_pci, sizeof(struct gem_pci_softc),
    gem_pci_match, gem_pci_attach, gem_pci_detach, NULL, NULL, NULL,
    DVF_DETACH_SHUTDOWN);

/*
 * Attach routines need to be split out to different bus-specific files.
 */

int
gem_pci_match(device_t parent, cfdata_t cf, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_SUN &&
	    (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_SUN_ERINETWORK ||
	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_SUN_GEMNETWORK))
		return (1);

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_APPLE &&
	    (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_GMAC ||
	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_GMAC2 ||
	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_GMAC3 ||
 	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_SHASTA_GMAC ||
	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_K2_GMAC ||
	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_SHASTA_GMAC ||
	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_INTREPID2_GMAC))
		return (1);


	return (0);
}

#if GEM_USE_LOCAL_MAC_ADDRESS
static inline int
gempromvalid(u_int8_t* buf)
{
	return buf[0] == 0x18 && buf[1] == 0x00 &&	/* structure length */
	    buf[2] == 0x00 &&				/* revision */
	    (buf[3] == 0x00 ||				/* hme */
	     buf[3] == 0x80) &&				/* qfe */
	    buf[4] == PCI_SUBCLASS_NETWORK_ETHERNET &&	/* subclass code */
	    buf[5] == PCI_CLASS_NETWORK;		/* class code */
}

static inline int
isshared_pins(u_int8_t* buf)
{
	return buf[0] == 's' && buf[1] == 'h' && buf[2] == 'a' &&
	    buf[3] == 'r' && buf[4] == 'e' && buf[5] == 'd' &&
	    buf[6] == '-' && buf[7] == 'p' && buf[8] == 'i' &&
	    buf[9] == 'n' && buf[10] == 's';
}
#endif

static inline int
isserdes(u_int8_t* buf)
{
	return buf[0] == 's' && buf[1] == 'e' && buf[2] == 'r' &&
	    buf[3] == 'd' && buf[4] == 'e' && buf[5] == 's';
}

void
gem_pci_attach(device_t parent, device_t self, void *aux)
{
	struct pci_attach_args *pa = aux;
	struct gem_pci_softc *gsc = device_private(self);
	struct gem_softc *sc = &gsc->gsc_gem;
	char devinfo[256];
	uint8_t enaddr[ETHER_ADDR_LEN];
#if GEM_USE_LOCAL_MAC_ADDRESS
	u_int8_t		*enp;
	bus_space_handle_t	romh;
	u_int8_t		buf[0x0800];
	int			dataoff, vpdoff, serdes;
#if GEM_USE_LOCAL_MAC_ADDRESS || defined(GEM_DEBUG)
	int i;
#endif
#ifdef GEM_DEBUG
	int j;
#endif
	struct pci_vpd		*vpd;
	static const u_int8_t promhdr[] = { 0x55, 0xaa };
#define PROMHDR_PTR_DATA	0x18
	static const u_int8_t promdat[] = {
		0x50, 0x43, 0x49, 0x52,		/* "PCIR" */
		PCI_VENDOR_SUN & 0xff, PCI_VENDOR_SUN >> 8,
		PCI_PRODUCT_SUN_GEMNETWORK & 0xff,
		PCI_PRODUCT_SUN_GEMNETWORK >> 8
	};
#define PROMDATA_PTR_VPD	0x08
#define PROMDATA_DATA2		0x0a
#endif  /* GEM_USE_LOCAL_MAC_ADDRESS */

	aprint_naive(": Ethernet controller\n");

	sc->sc_dev = self;
	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	sc->sc_chiprev = PCI_REVISION(pa->pa_class);
	aprint_normal(": %s (rev. 0x%02x)\n", devinfo, sc->sc_chiprev);

	/*
	 * Some Sun GEMs/ERIs do have their intpin register bogusly set to 0,
	 * although it should be 1. correct that.
	 */
	if (pa->pa_intrpin == 0)
		pa->pa_intrpin = 1;

	sc->sc_variant = GEM_UNKNOWN;

	sc->sc_dmatag = pa->pa_dmat;

	sc->sc_flags |= GEM_PCI;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_SUN) {
		if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_SUN_GEMNETWORK)
			sc->sc_variant = GEM_SUN_GEM;
		if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_SUN_ERINETWORK)
			sc->sc_variant = GEM_SUN_ERI;
	} else if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_APPLE) {
		if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_GMAC ||
		     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_GMAC2 ||
		     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_GMAC3 ||
		     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_SHASTA_GMAC ||
		     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_INTREPID2_GMAC)
			sc->sc_variant = GEM_APPLE_GMAC;
		if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_K2_GMAC)
			sc->sc_variant = GEM_APPLE_K2_GMAC;
	}

	if (sc->sc_variant == GEM_UNKNOWN) {
		aprint_error_dev(sc->sc_dev, "unknown adaptor\n");
		return;
	}

#define PCI_GEM_BASEADDR	(PCI_MAPREG_START + 0x00)

	/* XXX Need to check for a 64-bit mem BAR? */
	if (pci_mapreg_map(pa, PCI_GEM_BASEADDR,
	    PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_32BIT, 0,
	    &sc->sc_bustag, &sc->sc_h1, NULL, &sc->sc_size) != 0)
	{
		aprint_error_dev(sc->sc_dev, "unable to map device registers\n");
		return;
	}
	if (bus_space_subregion(sc->sc_bustag, sc->sc_h1,
	    GEM_PCI_BANK2_OFFSET, GEM_PCI_BANK2_SIZE, &sc->sc_h2)) {
		aprint_error_dev(sc->sc_dev, "unable to create bank 2 subregion\n");
		return;
	}

#if GEM_USE_LOCAL_MAC_ADDRESS
	/*
	 * Dig out VPD (vital product data) and acquire Ethernet address.
	 * The VPD of gem resides in the PCI PROM (PCI FCode).
	 */
	/*
	 * ``Writing FCode 3.x Programs'' (newer ones, dated 1997 and later)
	 * chapter 2 describes the data structure.
	 */

	enp = NULL;

	if (sc->sc_variant == GEM_SUN_GEM &&
	    (bus_space_subregion(sc->sc_bustag, sc->sc_h1,
	    GEM_PCI_ROM_OFFSET, GEM_PCI_ROM_SIZE, &romh)) == 0) {

		/* read PCI Expansion PROM Header */
		bus_space_read_region_1(sc->sc_bustag,
		    romh, 0, buf, sizeof buf);

		/* Check for "shared-pins = serdes" in FCode. */
		i = 0;
		serdes = 0;
		while (i < (sizeof buf) - sizeof "serdes") {
			if (!serdes) {
				if (isserdes(&buf[i]))
					serdes = 1;
			} else {
				if (isshared_pins(&buf[i]))
					serdes = 2;
			}
			if (serdes == 2) {
				sc->sc_flags |= GEM_SERDES;
				break;
			}
			i++;
		}
#ifdef GEM_DEBUG
		/* PROM dump */
		printf("%s: PROM dump (0x0000 to %04lx)\n", device_xname(sc->sc_dev),
		    (sizeof buf) - 1);
		i = 0;
		j = 0;
		printf("  %04x  ", i);
		while (i < sizeof buf) {
			printf("%02x ", buf[i]);
			if (i && !(i % 8))
				printf(" ");
			if (i && !(i % 16)) {
				printf(" ");
				while (j < i) {
					if (buf[j] > 31 && buf[j] < 128)
						printf("%c", buf[j]);
					else
						printf(".");
					j++;
				}
				j = i;
				printf("\n  %04x  ", i);
				}
			i++;
		}
		printf("\n");
#endif

		if (memcmp(buf, promhdr, sizeof promhdr) == 0 &&
		    (dataoff = (buf[PROMHDR_PTR_DATA] |
			(buf[PROMHDR_PTR_DATA + 1] << 8))) >= 0x1c) {

			/* read PCI Expansion PROM Data */
			bus_space_read_region_1(sc->sc_bustag, romh, dataoff,
			    buf, 64);
			if (memcmp(buf, promdat, sizeof promdat) == 0 &&
			    gempromvalid(buf + PROMDATA_DATA2) &&
			    (vpdoff = (buf[PROMDATA_PTR_VPD] |
				(buf[PROMDATA_PTR_VPD + 1] << 8))) >= 0x1c) {
	
				/*
				 * The VPD of gem is not in PCI 2.2 standard
				 * format.  The length in the resource header
				 * is in big endian, and resources are not
				 * properly terminated (only one resource
				 * and no end tag).
				 */
				/* read PCI VPD */
				bus_space_read_region_1(sc->sc_bustag, romh,
				    vpdoff, buf, 64);
				vpd = (void *)(buf + 3);
				if (PCI_VPDRES_ISLARGE(buf[0]) &&
				    PCI_VPDRES_LARGE_NAME(buf[0])
					== PCI_VPDRES_TYPE_VPD &&
				    vpd->vpd_key0 == 0x4e /* N */ &&
				    vpd->vpd_key1 == 0x41 /* A */ &&
				    vpd->vpd_len == ETHER_ADDR_LEN) {
					/*
					 * Ethernet address found
					 */
					enp = buf + 6;
				}
			}
		}
	}

	if (enp)
		memcpy(enaddr, enp, ETHER_ADDR_LEN);
	else
#endif  /* GEM_USE_LOCAL_MAC_ADDRESS */
#ifdef __sparc__
	{
		if (strcmp(prom_getpropstring(PCITAG_NODE(pa->pa_tag),
		    "shared-pins"), "serdes") == 0)
			sc->sc_flags |= GEM_SERDES;
		prom_getether(PCITAG_NODE(pa->pa_tag), enaddr);
	}
#else
#ifdef macppc
	{
		int node;
		char sp[6];	/* "serdes" */

		node = pcidev_to_ofdev(pa->pa_pc, pa->pa_tag);
		if (node == 0) {
			aprint_error_dev(sc->sc_dev, "unable to locate OpenFirmware node\n");
			return;
		}

		OF_getprop(node, "shared-pins", sp, sizeof(sp));
		if (isserdes(sp))
			sc->sc_flags |= GEM_SERDES;
		OF_getprop(node, "local-mac-address", enaddr, sizeof(enaddr));
	}
#else
		printf("%s: no Ethernet address found\n", device_xname(sc->sc_dev));
#endif /* macppc */
#endif /* __sparc__ */

	if (pci_intr_map(pa, &gsc->gsc_handle) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to map interrupt\n");
		return;
	}
	gsc->gsc_pc = pa->pa_pc;
	gem_pci_estintr(gsc);

	/* Finish off the attach. */
	gem_attach(sc, enaddr);

	if (!pmf_device_register1(sc->sc_dev, gem_pci_suspend, gem_pci_resume,
	                          gem_shutdown)) {
		aprint_error_dev(sc->sc_dev,
		    "could not establish power handlers\n");
	} else
		pmf_class_network_register(sc->sc_dev, &sc->sc_ethercom.ec_if);
}

static bool
gem_pci_suspend(device_t self PMF_FN_ARGS)
{
	struct gem_pci_softc *gsc = device_private(self);

	if (gsc->gsc_ih != NULL) {
		pci_intr_disestablish(gsc->gsc_pc, gsc->gsc_ih);
		gsc->gsc_ih = NULL;
	}

	return true;
}

static bool
gem_pci_estintr(struct gem_pci_softc *gsc)
{
	struct gem_softc *sc = &gsc->gsc_gem;
	const char *intrstr;

	intrstr = pci_intr_string(gsc->gsc_pc, gsc->gsc_handle);
	gsc->gsc_ih = pci_intr_establish(gsc->gsc_pc, gsc->gsc_handle, IPL_NET,
	    gem_intr, sc);
	if (gsc->gsc_ih == NULL) {
		aprint_error_dev(sc->sc_dev, "unable to establish interrupt");
		if (intrstr != NULL)
			aprint_normal(" at %s", intrstr);
		aprint_normal("\n");
		return false;
	}
	aprint_normal_dev(sc->sc_dev, "interrupting at %s\n", intrstr);
	return true;
}

static bool
gem_pci_resume(device_t self PMF_FN_ARGS)
{
	struct gem_pci_softc *gsc = device_private(self);

	return gem_pci_estintr(gsc);
}

static int
gem_pci_detach(device_t self, int flags)
{
	int rc;
	struct gem_pci_softc *gsc = device_private(self);
	struct gem_softc *sc = &gsc->gsc_gem;

	switch (sc->sc_att_stage) {
	case GEM_ATT_BACKEND_2:
		pmf_device_deregister(self);
		sc->sc_att_stage = GEM_ATT_FINISHED;
		/*FALLTHROUGH*/
	default:
		if ((rc = gem_detach(sc, flags)) != 0)
			return rc;
		/*FALLTHROUGH*/
	case GEM_ATT_BACKEND_1:
		if (gsc->gsc_ih != NULL)
			pci_intr_disestablish(gsc->gsc_pc, gsc->gsc_ih);

		bus_space_unmap(sc->sc_bustag, sc->sc_h1, sc->sc_size);
		/*FALLTHROUGH*/
	case GEM_ATT_BACKEND_0:
		sc->sc_att_stage = GEM_ATT_BACKEND_0;
		break;
	}
	return 0;
}
