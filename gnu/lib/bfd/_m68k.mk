# $NetBSD: _m68k.mk,v 1.2 1997/10/17 20:07:15 gwr Exp $

BFD_MACHINES =	cpu-m68k.c
BFD_BACKENDS =	m68knetbsd.c m68k4knetbsd.c sunos.c aout32.c

ARCH_DEFS = -DARCH_m68k \
 -DSELECT_ARCHITECTURES='&bfd_m68k_arch'

TDEFAULTS = \
 -DDEFAULT_VECTOR=m68knetbsd_vec \
 -DSELECT_VECS=' &m68knetbsd_vec, &m68k4knetbsd_vec, &sunos_big_vec ' \
 -DHAVE_m68knetbsd_vec \
 -DHAVE_m68k4knetbsd_vec \
 -DHAVE_sunos_big_vec \
 -DNETBSD_CORE

OPCODE_MACHINES =  m68k-dis.c m68k-opc.c
