#	$NetBSD: _alpha.mk,v 1.5 1999/02/02 22:16:57 tv Exp $

BFD_ARCH_SRCS=	cpu-alpha.c \
		elf64-alpha.c elf64.c elf.c elflink.c dwarf2.c \
		coff-alpha.c ecoff.c ecofflink.c \
		alpha-dis.c alpha-opc.c

BFD_ARCH_DEFS=	-DARCH_alpha \
		-DSELECT_ARCHITECTURES='&bfd_alpha_arch' \
		-DDEFAULT_VECTOR=bfd_elf64_alpha_vec \
		-DSELECT_VECS='&bfd_elf64_alpha_vec, &ecoffalpha_little_vec' \
		-DHAVE_bfd_elf64_alpha_vec \
		-DHAVE_ecoffalpha_little_vec \
		-DNETBSD_CORE
