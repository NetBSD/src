/* $NetBSD: bus.h,v 1.1.22.1 2020/04/13 08:04:05 martin Exp $ */

#ifndef _RISCV_BUS_H_
typedef paddr_t bus_addr_t;
typedef psize_t bus_size_t;

#define PRIxBUSADDR	PRIxPADDR
#define PRIxBUSSIZE	PRIxPSIZE
#define PRIuBUSSIZE	PRIuPSIZE

typedef struct riscv_bus_space_tag *bus_space_tag_t;
typedef uintptr_t bus_space_handle_t;

#define PRIxBSH		PRIxPTR

#endif
