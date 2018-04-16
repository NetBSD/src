/*	$NetBSD: t_api_dhcp.c,v 1.2.2.2 2018/04/16 01:59:49 pgoyette Exp $	*/

/*
 * We have to have a number of symbols defined in order to build a
 * DHCP program.
 */

#include <config.h>
#include "dhcpd.h"

void 
bootp(struct packet *packet) {
}

void
dhcp(struct packet *packet) {
}

void
dhcpv6(struct packet *packet) {
}

isc_result_t
dhcpv4o6_handler(omapi_object_t *h) {
	return ISC_R_NOTIMPLEMENTED;
}

isc_result_t
dhcp_set_control_state(control_object_state_t old, control_object_state_t new) {
	return ISC_R_NOTIMPLEMENTED;
}

int
check_collection(struct packet *p, struct lease *l, struct collection *c) {
	return 0;
}

void
classify (struct packet *p, struct class *c) {
}

isc_result_t
find_class(struct class **class, const char *c1, const char *c2, int i) {
        return ISC_R_NOTFOUND;
}

int
parse_allow_deny(struct option_cache **oc, struct parse *p, int i) {
        return 0;
}

