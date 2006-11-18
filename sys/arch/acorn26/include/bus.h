/* $NetBSD: bus.h,v 1.2.60.1 2006/11/18 21:28:58 ad Exp $ */

#ifndef _ACORN26_BUS_H
#define _ACORN26_BUS_H

#include <arm/bus.h>

extern struct bus_space iobus_bs_tag;

extern int bus_space_shift(bus_space_tag_t, int, bus_space_tag_t *);

#endif
