# $NetBSD: _sparc.mk,v 1.1.1.1 1997/09/26 02:38:49 gwr Exp $

BFD_BACKENDS =	sparcnetbsd.c sunos.c aout32.c
BFD_MACHINES =	cpu-sparc.c

TDEFAULTS = \
 -DSELECT_ARCHITECTURES='&bfd_sparc_arch' \
 -DDEFAULT_VECTOR=sparcnetbsd_vec \
 -DSELECT_VECS=' &sparcnetbsd_vec, &sunos_big_vec ' \
 -DHAVE_sparcnetbsd_vec \
 -DHAVE_sunos_big_vec \
 -DNETBSD_CORE

OPCODE_MACHINES =  sparc-dis.c sparc-opc.c
