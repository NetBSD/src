# $NetBSD: _sparc.mk,v 1.3 1998/07/01 02:12:11 tv Exp $

BFD_MACHINES =	cpu-sparc.c
BFD_BACKENDS =	sparcnetbsd.c sunos.c aout32.c \
		elf32-sparc.c elf32.c \
		elf.c elflink.c

ARCH_DEFS = -DARCH_sparc \
 -DSELECT_ARCHITECTURES='&bfd_sparc_arch'

TDEFAULTS = \
 -DDEFAULT_VECTOR=sparcnetbsd_vec \
 -DSELECT_VECS=' &sparcnetbsd_vec, &sunos_big_vec, &bfd_elf32_sparc_vec ' \
 -DHAVE_sparcnetbsd_vec \
 -DHAVE_sunos_big_vec \
 -DHAVE_bfd_elf32_sparc_vec \
 -DNETBSD_CORE

OPCODE_MACHINES =  sparc-dis.c sparc-opc.c
