/* $NetBSD: eapvar.h,v 1.1.2.2 2004/08/03 10:49:06 skrll Exp $ */

struct eap_gameport_args {
	bus_space_tag_t gpa_iot;
	bus_space_handle_t gpa_ioh;
};

struct device *eap_joy_attach(struct device *, struct eap_gameport_args *);
int eap_joy_detach(struct device *, struct eap_gameport_args *);
