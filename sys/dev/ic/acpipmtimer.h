/* $NetBSD: acpipmtimer.h,v 1.1 2006/06/26 16:13:21 drochner Exp $ */

int acpipmtimer_attach(struct device *,
		       bus_space_tag_t, bus_space_handle_t, bus_size_t, int);
#define ACPIPMT_32BIT 1
#define ACPIPMT_BADLATCH 2
