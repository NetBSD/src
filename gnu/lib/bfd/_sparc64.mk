#	$NetBSD: _sparc64.mk,v 1.2 1999/02/02 22:16:58 tv Exp $

BFD_ARCH_SRCS=	cpu-sparc.c \
		elf32-sparc.c elf32.c \
		elf64-sparc.c elf64.c \
		elf.c elflink.c dwarf2.c \
		sparc-dis.c sparc-opc.c

BFD_ARCH_DEFS=	-DARCH_sparc \
		-DSELECT_ARCHITECTURES='&bfd_sparc_arch' \
		-DDEFAULT_VECTOR=bfd_elf64_sparc_vec \
		-DSELECT_VECS='&bfd_elf64_sparc_vec,&bfd_elf32_sparc_vec' \
		-DHAVE_bfd_elf64_sparc_vec \
		-DHAVE_bfd_elf32_sparc_vec \
		-DNETBSD_CORE


