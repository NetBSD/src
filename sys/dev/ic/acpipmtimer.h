/* $NetBSD: acpipmtimer.h,v 1.1.66.2 2009/08/19 18:47:06 yamt Exp $ */

typedef void *acpipmtimer_t;

acpipmtimer_t acpipmtimer_attach(device_t,
		       bus_space_tag_t, bus_space_handle_t, bus_size_t, int);
int acpipmtimer_detach(acpipmtimer_t, int);
#define ACPIPMT_32BIT 1
#define ACPIPMT_BADLATCH 2
