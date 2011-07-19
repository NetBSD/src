/* $NetBSD: bus_funcs.h,v 1.1 2011/07/19 16:05:10 dyoung Exp $ */

#ifndef _ACORN26_BUS_FUNCS_H_
#define _ACORN26_BUS_FUNCS_H_

#include <arm/bus_funcs.h>

int bus_space_shift(bus_space_tag_t, int, bus_space_tag_t *);

#endif /* _ACORN26_BUS_FUNCS_H_ */
