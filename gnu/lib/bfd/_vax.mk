# $NetBSD: _vax.mk,v 1.2 1997/10/17 20:07:48 gwr Exp $

BFD_MACHINES =	cpu-vax.c
BFD_BACKENDS =	vaxnetbsd.c aout32.c

ARCH_DEFS = -DARCH_vax \
 -DSELECT_ARCHITECTURES='&bfd_arch_vax'

TDEFAULTS = \
 -DDEFAULT_VECTOR=vaxnetbsd_vec \
 -DSELECT_VECS=' &vaxnetbsd_vec ' \
 -DHAVE_vaxnetbsd_vec \
 -DNETBSD_CORE

OPCODE_MACHINES = # XXX not yet
