/* $NetBSD: bus.h,v 1.1 2014/09/19 17:36:26 matt Exp $ */

#ifndef _RISCV_BUS_H_
typedef paddr_t bus_addr_t;
typedef psize_t bus_size_t;
typedef struct riscv_bus_space_tag *bus_space_tag_t;
typedef uintptr_t bus_space_handle_t;
#endif
