# $NetBSD: _vax.mk,v 1.1.1.1 1997/09/26 02:38:49 gwr Exp $

BFD_BACKENDS =	vaxnetbsd.c aout32.c
BFD_MACHINES =	cpu-vax.c

TDEFAULTS = \
 -DSELECT_ARCHITECTURES='&bfd_arch_vax' \
 -DDEFAULT_VECTOR=vaxnetbsd_vec \
 -DSELECT_VECS=' &vaxnetbsd_vec ' \
 -DHAVE_vaxnetbsd_vec \
 -DNETBSD_CORE

OPCODE_MACHINES = # XXX not yet
