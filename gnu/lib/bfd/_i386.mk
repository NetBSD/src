# $NetBSD: _i386.mk,v 1.1.1.1 1997/09/26 02:38:49 gwr Exp $

BFD_BACKENDS =	i386netbsd.c aout32.c
BFD_MACHINES =	cpu-i386.c

TDEFAULTS = \
 -DSELECT_ARCHITECTURES='&bfd_i386_arch' \
 -DDEFAULT_VECTOR=i386netbsd_vec \
 -DSELECT_VECS=' &i386netbsd_vec ' \
 -DHAVE_i386netbsd_vec \
 -DNETBSD_CORE

OPCODE_MACHINES =  i386-dis.c
