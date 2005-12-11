/* $NetBSD: eapvar.h,v 1.2 2005/12/11 12:22:49 christos Exp $ */

struct eap_gameport_args {
	bus_space_tag_t gpa_iot;
	bus_space_handle_t gpa_ioh;
};

struct device *eap_joy_attach(struct device *, struct eap_gameport_args *);
int eap_joy_detach(struct device *, struct eap_gameport_args *);
