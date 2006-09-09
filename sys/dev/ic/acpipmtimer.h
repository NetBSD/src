/* $NetBSD: acpipmtimer.h,v 1.1.10.2 2006/09/09 02:50:00 rpaulo Exp $ */

int acpipmtimer_attach(struct device *,
		       bus_space_tag_t, bus_space_handle_t, bus_size_t, int);
#define ACPIPMT_32BIT 1
#define ACPIPMT_BADLATCH 2
