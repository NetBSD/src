#ifndef _ARM_IMX_IMXCLOCKVAR_H
#define	_ARM_IMX_IMXCLOCKVAR_H

struct imxclock_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	int sc_intr;

	int sc_reload_value;

	void *sc_ih;			/* interrupt handler */
};

extern struct imxclock_softc *epit1_sc, *epit2_sc;

int imxclock_get_timerfreq(struct imxclock_softc *);
#endif	/* _ARM_IMX_IMXCLOCKVAR_H */
