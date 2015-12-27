extern struct fdtbus_gpio_controller_func exynos_gpio_funcs;

struct exynos_pinctrl_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

};
