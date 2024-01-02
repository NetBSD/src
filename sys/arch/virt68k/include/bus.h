/*	$NetBSD: bus.h,v 1.1 2024/01/02 07:41:00 thorpej Exp $	*/

#include <machine/bus_space.h>
#include <machine/bus_dma.h>

#define generic_btop(x) m68k_btop(x)
