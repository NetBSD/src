# $NetBSD: _powerpc.mk,v 1.2 1997/10/17 20:07:33 gwr Exp $

BFD_MACHINES =	cpu-powerpc.c
BFD_BACKENDS =	elf32-ppc.c elf32.c elf.c elflink.c

ARCH_DEFS = -DARCH_powerpc \
 -DSELECT_ARCHITECTURES='&bfd_powerpc_arch'

TDEFAULTS = \
 -DDEFAULT_VECTOR=bfd_elf32_powerpc_vec \
 -DSELECT_VECS=' &bfd_elf32_powerpc_vec, &bfd_elf32_powerpcle_vec ' \
 -DHAVE_bfd_elf32_powerpc_vec \
 -DHAVE_bfd_elf32_powerpcle_vec \
 -DNETBSD_CORE

OPCODE_MACHINES =  ppc-dis.c ppc-opc.c
