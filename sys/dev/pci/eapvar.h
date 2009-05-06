/* $NetBSD: eapvar.h,v 1.3 2009/05/06 10:34:32 cegger Exp $ */

struct eap_gameport_args {
	bus_space_tag_t gpa_iot;
	bus_space_handle_t gpa_ioh;
};

device_t eap_joy_attach(device_t, struct eap_gameport_args *);
int eap_joy_detach(device_t, struct eap_gameport_args *);
