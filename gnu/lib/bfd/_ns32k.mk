# $NetBSD: _ns32k.mk,v 1.2.2.1 1997/10/29 04:12:38 mellon Exp $

BFD_BACKENDS =	ns32knetbsd.c aout-ns32k.c
BFD_MACHINES =	cpu-ns32k.c

ARCH_DEFS = -DARCH_ns32k \
 -DSELECT_ARCHITECTURES='&bfd_ns32k_arch'

TDEFAULTS = \
 -DDEFAULT_VECTOR=pc532netbsd_vec \
 -DSELECT_VECS=' &pc532netbsd_vec ' \
 -DHAVE_pc532netbsd_vec \
 -DNETBSD_CORE

OPCODE_MACHINES =  ns32k-dis.c
