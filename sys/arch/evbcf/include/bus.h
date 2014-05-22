/*	$NetBSD: bus.h,v 1.2.4.2 2014/05/22 11:39:44 yamt Exp $ */

#include <machine/bus_space.h>
#include <machine/bus_dma.h>

#define generic_btop(x) m68k_btop(x)
