/* $NetBSD: bus.h,v 1.3 2006/09/30 16:30:10 bjh21 Exp $ */

#ifndef _ACORN26_BUS_H
#define _ACORN26_BUS_H

#include <arm/bus.h>

extern struct bus_space iobus_bs_tag;

extern int bus_space_shift(bus_space_tag_t, int, bus_space_tag_t *);

#endif
