/*
 * RealTek 8139C+/8169/8169S/8110S PCI NIC driver
 *
 * Written by Bill Paul <wpaul@windriver.com>
 * Senior Networking Software Engineer
 * Wind River Systems
 *
 * NetBSD bus-specific frontends for written by
 * Jonathan Stone <jonathan@netbsd.org>
 */

#include "bpfilter.h"
#include "vlan.h"

#include <sys/types.h>

#include <machine/bus.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <sys/param.h>
#include <sys/endian.h>
#include <sys/systm.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <net/if_vlanvar.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

/*
 * Default to using PIO access for this driver.
 */
#define RE_USEIOSPACE

#include <dev/ic/rtl81x9reg.h>
#include <dev/ic/rtl81x9var.h>
#include <dev/ic/rtl8169var.h>

struct re_pci_softc {
	struct rtk_softc sc_rtk;

	void *sc_ih;
	pci_chipset_tag_t sc_pc;
	pcitag_t sc_pcitag;
};

static int	re_pci_probe(struct device *, struct cfdata *, void *);
static void	re_pci_attach(struct device *, struct device *, void *);

/*
 * Various supported device vendors/types and their names.
 */
static struct rtk_type re_devs[] = {
	{ PCI_VENDOR_REALTEK, PCI_PRODUCT_REALTEK_RT8139, RTK_HWREV_8139CPLUS,
		"RealTek 8139C+ 10/100BaseTX" },
	{ PCI_VENDOR_REALTEK, PCI_PRODUCT_REALTEK_RT8169, RTK_HWREV_8169,
		"RealTek 8169 Gigabit Ethernet" },
	{ PCI_VENDOR_REALTEK, PCI_PRODUCT_REALTEK_RT8169, RTK_HWREV_8169S,
		"RealTek 8169S Single-chip Gigabit Ethernet" },
	{ PCI_VENDOR_REALTEK, PCI_PRODUCT_REALTEK_RT8169, RTK_HWREV_8110S,
		"RealTek 8110S Single-chip Gigabit Ethernet" },
	{ 0, 0, 0, NULL }
};

static struct rtk_hwrev re_hwrevs[] = {
	{ RTK_HWREV_8139, RTK_8139,  "" },
	{ RTK_HWREV_8139A, RTK_8139, "A" },
	{ RTK_HWREV_8139AG, RTK_8139, "A-G" },
	{ RTK_HWREV_8139B, RTK_8139, "B" },
	{ RTK_HWREV_8130, RTK_8139, "8130" },
	{ RTK_HWREV_8139C, RTK_8139, "C" },
	{ RTK_HWREV_8139D, RTK_8139, "8139D/8100B/8100C" },
	{ RTK_HWREV_8139CPLUS, RTK_8139CPLUS, "C+"},
	{ RTK_HWREV_8169, RTK_8169, "8169"},
	{ RTK_HWREV_8169S, RTK_8169, "8169S"},
	{ RTK_HWREV_8110S, RTK_8169, "8110S"},
	{ RTK_HWREV_8100, RTK_8139, "8100"},
	{ RTK_HWREV_8101, RTK_8139, "8101"},
	{ 0, 0, NULL }
};

CFATTACH_DECL(re_pci, sizeof(struct re_pci_softc), re_pci_probe, re_pci_attach,
	      NULL, NULL);

/*
 * Probe for a RealTek 8139C+/8169/8110 chip. Check the PCI vendor and device
 * IDs against our list and return a device name if we find a match.
 */
static int
re_pci_probe(struct device *parent, struct cfdata *match, void *aux)
{
	struct rtk_type		*t;
	struct pci_attach_args	*pa = aux;
	bus_space_tag_t		rtk_btag;
	bus_space_handle_t	rtk_bhandle;
	bus_size_t		bsize;
	u_int32_t		hwrev;

	t = re_devs;

	while(t->rtk_name != NULL) {
		if ((PCI_VENDOR(pa->pa_id) == t->rtk_vid) &&
		    (PCI_PRODUCT(pa->pa_id) == t->rtk_did)) {

			/*
			 * Temporarily map the I/O space
			 * so we can read the chip ID register.
			 */
			if (pci_mapreg_map(pa, RTK_PCI_LOIO,
			    PCI_MAPREG_TYPE_IO, 0, &rtk_btag,
			    &rtk_bhandle, NULL, &bsize)) {
				printf("can't map i/o space\n");
				return 0;
			}
			hwrev = bus_space_read_4(rtk_btag, rtk_bhandle,
			    RTK_TXCFG) & RTK_TXCFG_HWREV;
			bus_space_unmap(rtk_btag, rtk_bhandle, bsize);
			if (t->rtk_basetype == hwrev)
				return 2;	/* defeat rtk(4) */
		}
		t++;
	}

	return 0;
}

static void
re_pci_attach(struct device *parent, struct device *self, void *aux)
{
#if 0
	u_char			eaddr[ETHER_ADDR_LEN];
	u_int16_t		val;
	struct rtk_hwrev	*hw_rev;
	int 	i, addr_len;
#endif
	struct re_pci_softc	*psc = (void *)self;
	struct rtk_softc	*sc = &psc->sc_rtk;
	struct pci_attach_args 	*pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	pci_intr_handle_t ih;
	const char *intrstr = NULL;
	struct ifnet		*ifp;
	struct rtk_type		*t;
	struct rtk_hwrev	*hw_rev;
	int			hwrev;
	int			error = 0;
	pcireg_t		command;


#if 0 /*ndef BURN_BRIDGES*/
	/*
	 * Handle power management nonsense.
	 */

	if (pci_get_powerstate(dev) != PCI_POWERSTATE_D0) {
		u_int32_t		iobase, membase, irq;

		/* Save important PCI config data. */
		iobase = pci_read_config(dev, RTK_PCI_LOIO, 4);
		membase = pci_read_config(dev, RTK_PCI_LOMEM, 4);
		irq = pci_read_config(dev, RTK_PCI_INTLINE, 4);

		/* Reset the power state. */
		printf("%s: chip is is in D%d power mode "
		    "-- setting to D0\n", unit,
		    pci_get_powerstate(dev));

		pci_set_powerstate(dev, PCI_POWERSTATE_D0);

		/* Restore PCI config data. */
		pci_write_config(dev, RTK_PCI_LOIO, iobase, 4);
		pci_write_config(dev, RTK_PCI_LOMEM, membase, 4);
		pci_write_config(dev, RTK_PCI_INTLINE, irq, 4);
	}
#endif
	/*
	 * Map control/status registers.
	 */
	command = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	command |= PCI_COMMAND_MASTER_ENABLE;
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG, command);

#ifdef RE_USEIOSPACE
	if (pci_mapreg_map(pa, RTK_PCI_LOIO, PCI_MAPREG_TYPE_IO, 0,
	    &sc->rtk_btag, &sc->rtk_bhandle, NULL, NULL)) {
		printf("%s: can't map i/o space\n", sc->sc_dev.dv_xname);
		error = ENXIO;
		goto fail;
	}
#else
	if (pci_mapreg_map(pa, RTK_PCI_LOMEM, PCI_MAPREG_TYPE_MEM, 0,
	    &sc->rtk_btag, &sc->rtk_bhandle, NULL, NULL)) {
		printf("%s: can't map mem space\n", sc->sc_dev.dv_xname);
		error = ENXIO;
		goto fail;
	}
#endif
	t = re_devs;
	hwrev = CSR_READ_4(sc, RTK_TXCFG) & RTK_TXCFG_HWREV;

	while(t->rtk_name != NULL) {
		if ((PCI_VENDOR(pa->pa_id) == t->rtk_vid) &&
		    (PCI_PRODUCT(pa->pa_id) == t->rtk_did)) {

			if (t->rtk_basetype == hwrev)
				break;
		}
		t++;
	}

	hw_rev = re_hwrevs;
	hwrev = CSR_READ_4(sc, RTK_TXCFG) & RTK_TXCFG_HWREV;
	while (hw_rev->rtk_desc != NULL) {
		if (hw_rev->rtk_rev == hwrev) {
			sc->rtk_type = hw_rev->rtk_type;
			break;
		}
		hw_rev++;
	}

	sc->sc_dmat = pa->pa_dmat;

	/*
	 * No power/enable/disable machinery for PCI attac;
	 * mark the card enabled now.
	 */
	sc->sc_flags |= RTK_ENABLED;

	re_attach(sc);

	ifp = &sc->ethercom.ec_if;
	
	/* Hook interrupt last to avoid having to lock softc */
	/* Allocate interrupt */
	if (pci_intr_map(pa, &ih)) {
		printf("%s: couldn't map interrupt\n", sc->sc_dev.dv_xname);
		error = ENXIO;
		ether_ifdetach(ifp);
		if_detach(ifp);
		goto fail;
	}
	intrstr = pci_intr_string(pc, ih);
	psc->sc_ih = pci_intr_establish(pc, ih, IPL_NET, re_intr, sc);
	if (psc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt",
		    sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		ether_ifdetach(ifp);
		if_detach(ifp);
		return;
	}
	aprint_normal("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);

fail:
#if 0
	if (error)
		re_detach(sc);
#endif
	return;
}
