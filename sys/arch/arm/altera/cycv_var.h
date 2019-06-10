/* $NetBSD: cycv_var.h,v 1.1.6.2 2019/06/10 22:05:50 christos Exp $ */
#ifndef _ARM_ALTERA_CYCV_VAR_H
#define _ARM_ALTERA_CYCV_VAR_H

#include <sys/types.h>
#include <sys/bus.h>

extern struct bus_space armv7_generic_bs_tag;
extern struct bus_space armv7_generic_a4x_bs_tag;
extern struct arm32_bus_dma_tag arm_generic_dma_tag;

void cycv_bootstrap(void);
void cycv_cpuinit(void);

uint32_t cycv_clkmgr_early_get_mpu_clk(void);
uint32_t cycv_clkmgr_early_get_l4_sp_clk(void);

#endif /* _ARM_ALTERA_CYCV_VAR_H */
