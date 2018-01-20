/*	$NetBSD: gtvar.h,v 1.1 2018/01/20 13:56:09 skrll Exp $ */

#ifndef _GTVAR_H_
#define	_GTVAR_H_

#include <machine/bus_defs.h>

extern struct mips_bus_space gt_iot;
extern struct mips_bus_space gt_memt;

void    gt_bus_io_init(bus_space_tag_t, void *);
void    gt_bus_mem_init(bus_space_tag_t, void *);

#endif /* _GTVAR_H_ */
