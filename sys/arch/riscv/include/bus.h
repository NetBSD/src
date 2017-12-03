/* $NetBSD: bus.h,v 1.1.18.2 2017/12/03 11:36:39 jdolecek Exp $ */

#ifndef _RISCV_BUS_H_
typedef paddr_t bus_addr_t;
typedef psize_t bus_size_t;
typedef struct riscv_bus_space_tag *bus_space_tag_t;
typedef uintptr_t bus_space_handle_t;
#endif
