# $NetBSD: _ns32k.mk,v 1.3 1997/10/29 03:45:40 phil Exp $

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
