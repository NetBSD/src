#	$NetBSD: _alpha.mk,v 1.4 1998/08/22 19:02:10 tv Exp $

BFD_ARCH_SRCS=	cpu-alpha.c \
		elf64-alpha.c elf64.c elf.c elflink.c \
		coff-alpha.c ecoff.c ecofflink.c \
		alpha-dis.c alpha-opc.c

BFD_ARCH_DEFS=	-DARCH_alpha \
		-DSELECT_ARCHITECTURES='&bfd_alpha_arch' \
		-DDEFAULT_VECTOR=bfd_elf64_alpha_vec \
		-DSELECT_VECS='&bfd_elf64_alpha_vec, &ecoffalpha_little_vec' \
		-DHAVE_bfd_elf64_alpha_vec \
		-DHAVE_ecoffalpha_little_vec \
		-DNETBSD_CORE
