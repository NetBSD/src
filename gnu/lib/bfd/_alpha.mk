# $NetBSD: _alpha.mk,v 1.3 1998/08/21 21:11:02 tv Exp $

BFD_MACHINES =	cpu-alpha.c
BFD_BACKENDS =	elf64-alpha.c elf64.c elf.c elflink.c \
		coff-alpha.c ecoff.c ecofflink.c

ARCH_DEFS = -DARCH_alpha \
 -DSELECT_ARCHITECTURES='&bfd_alpha_arch'

TDEFAULTS = \
 -DDEFAULT_VECTOR=bfd_elf64_alpha_vec \
 -DSELECT_VECS=' &bfd_elf64_alpha_vec, &ecoffalpha_little_vec ' \
 -DHAVE_bfd_elf64_alpha_vec \
 -DHAVE_ecoffalpha_little_vec \
 -DNETBSD_CORE

OPCODE_MACHINES =  alpha-dis.c alpha-opc.c

BFD_WORDSIZE=	64
BFD_64BIT_LONG=	1
