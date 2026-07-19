/*	$NetBSD: bus.h,v 1.1 2026/07/19 01:48:21 thorpej Exp $	*/

#include <machine/bus_space.h>
#include <machine/bus_dma.h>

#define generic_btop(x) m68k_btop(x)
