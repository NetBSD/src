# $NetBSD: _i386.mk,v 1.3 1998/07/18 01:00:17 thorpej Exp $

BFD_MACHINES =	cpu-i386.c
BFD_BACKENDS =	i386netbsd.c aout32.c \
		elf32-i386.c elf32.c elf.c elflink.c

ARCH_DEFS = -DARCH_i386 \
 -DSELECT_ARCHITECTURES='&bfd_i386_arch'

TDEFAULTS = \
 -DDEFAULT_VECTOR=i386netbsd_vec \
 -DSELECT_VECS=' &i386netbsd_vec, &bfd_elf32_i386_vec ' \
 -DHAVE_i386netbsd_vec \
 -DHAVE_bfd_elf32_i386_vec \
 -DNETBSD_CORE

OPCODE_MACHINES =  i386-dis.c
