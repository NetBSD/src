/* $NetBSD: eapvar.h,v 1.1.2.4 2004/09/21 13:31:01 skrll Exp $ */

struct eap_gameport_args {
	bus_space_tag_t gpa_iot;
	bus_space_handle_t gpa_ioh;
};

struct device *eap_joy_attach(struct device *, struct eap_gameport_args *);
int eap_joy_detach(struct device *, struct eap_gameport_args *);
