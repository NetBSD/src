struct spic_softc {
	struct device sc_dev;

	bus_space_tag_t	sc_iot;
	bus_space_handle_t sc_ioh;

	struct callout sc_poll;

	int sc_buttons;
	char sc_enabled;

	struct device *sc_wsmousedev;
};

void spic_attach(struct spic_softc *);

int spic_intr(void *);
