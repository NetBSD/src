#ifndef _ARM_IMX_IMXUSBVAR_H
#define _ARM_IMX_IMXUSBVAR_H

struct imxehci_softc;

enum imx_usb_role {
	IMXUSB_HOST,
	IMXUSB_DEVICE
};

struct imxusbc_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	/* filled in by platform dependent routine */
	void (* sc_init_md_hook)(struct imxehci_softc *);
	void (* sc_setup_md_hook)(struct imxehci_softc *, enum imx_usb_role);
};

struct imxusbc_attach_args {
	bus_space_tag_t aa_iot;
	bus_space_handle_t aa_ioh;
	bus_dma_tag_t aa_dmat;
	int aa_unit;	/* 0: OTG, 1: HOST1, 2: HOST2 ... */
	int aa_irq;
};

enum imx_usb_if {
	IMXUSBC_IF_UTMI,
	IMXUSBC_IF_PHILIPS,
	IMXUSBC_IF_ULPI,
	IMXUSBC_IF_SERIAL,
	IMXUSBC_IF_UTMI_WIDE
};

struct imxehci_softc {
	ehci_softc_t sc_hsc;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	struct imxusbc_softc *sc_usbc;
	uint sc_unit;
	enum imx_usb_if sc_iftype;
};

int imxusbc_attach_common(device_t, device_t, bus_space_tag_t);
void imxehci_reset(struct imxehci_softc *);

#endif	/* _ARM_IMX_IMXUSBVAR_H */
