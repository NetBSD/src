# $NetBSD: _i386.mk,v 1.2 1997/10/17 20:07:07 gwr Exp $

BFD_MACHINES =	cpu-i386.c
BFD_BACKENDS =	i386netbsd.c aout32.c

ARCH_DEFS = -DARCH_i386 \
 -DSELECT_ARCHITECTURES='&bfd_i386_arch'

TDEFAULTS = \
 -DDEFAULT_VECTOR=i386netbsd_vec \
 -DSELECT_VECS=' &i386netbsd_vec ' \
 -DHAVE_i386netbsd_vec \
 -DNETBSD_CORE

OPCODE_MACHINES =  i386-dis.c
