# $NetBSD: _sparc.mk,v 1.2 1997/10/17 20:07:40 gwr Exp $

BFD_MACHINES =	cpu-sparc.c
BFD_BACKENDS =	sparcnetbsd.c sunos.c aout32.c

ARCH_DEFS = -DARCH_sparc \
 -DSELECT_ARCHITECTURES='&bfd_sparc_arch'

TDEFAULTS = \
 -DDEFAULT_VECTOR=sparcnetbsd_vec \
 -DSELECT_VECS=' &sparcnetbsd_vec, &sunos_big_vec ' \
 -DHAVE_sparcnetbsd_vec \
 -DHAVE_sunos_big_vec \
 -DNETBSD_CORE

OPCODE_MACHINES =  sparc-dis.c sparc-opc.c
