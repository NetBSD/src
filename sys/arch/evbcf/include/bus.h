/*	$NetBSD: bus.h,v 1.2 2014/03/18 18:20:41 riastradh Exp $ */

#include <machine/bus_space.h>
#include <machine/bus_dma.h>

#define generic_btop(x) m68k_btop(x)
