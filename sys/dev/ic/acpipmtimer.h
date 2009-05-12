/* $NetBSD: acpipmtimer.h,v 1.2 2009/05/12 14:25:17 cegger Exp $ */

int acpipmtimer_attach(device_t,
		       bus_space_tag_t, bus_space_handle_t, bus_size_t, int);
#define ACPIPMT_32BIT 1
#define ACPIPMT_BADLATCH 2
