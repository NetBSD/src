# $NetBSD: _ns32k.mk,v 1.2 1997/10/17 20:07:27 gwr Exp $

BFD_BACKENDS =	ns32knetbsd.c aout-ns32k.c
BFD_MACHINES =	cpu-ns32k.c

ARCH_DEFS = -DARCH_ns32k \
 -DSELECT_ARCHITECTURES='&bfd_ns32k_arch'

TDEFAULTS = \
 -DDEFAULT_VECTOR=ns32knetbsd_vec \
 -DSELECT_VECS=' &ns32knetbsd_vec ' \
 -DHAVE_ns32knetbsd_vec \
 -DNETBSD_CORE

OPCODE_MACHINES =  ns32k-dis.c
