# $NetBSD: _m68k.mk,v 1.1.1.1 1997/09/26 02:38:49 gwr Exp $

BFD_BACKENDS =	m68knetbsd.c m68k4knetbsd.c sunos.c aout32.c
BFD_MACHINES =	cpu-m68k.c

TDEFAULTS = \
 -DSELECT_ARCHITECTURES='&bfd_m68k_arch' \
 -DDEFAULT_VECTOR=m68knetbsd_vec \
 -DSELECT_VECS=' &m68knetbsd_vec, &m68k4knetbsd_vec, &sunos_big_vec ' \
 -DHAVE_m68knetbsd_vec \
 -DHAVE_m68k4knetbsd_vec \
 -DHAVE_sunos_big_vec \
 -DNETBSD_CORE

OPCODE_MACHINES =  m68k-dis.c m68k-opc.c
