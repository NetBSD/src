#	$NetBSD: _sparc64.mk,v 1.1 1998/11/23 09:23:30 mrg Exp $

BFD_ARCH_SRCS=	cpu-sparc.c \
		elf32-sparc.c elf32.c \
		elf64-sparc.c elf64.c \
		elf.c elflink.c \
		sparc-dis.c sparc-opc.c

BFD_ARCH_DEFS=	-DARCH_sparc \
		-DSELECT_ARCHITECTURES='&bfd_sparc_arch' \
		-DDEFAULT_VECTOR=bfd_elf64_sparc_vec \
		-DSELECT_VECS='&bfd_elf64_sparc_vec,&bfd_elf32_sparc_vec' \
		-DHAVE_bfd_elf64_sparc_vec \
		-DHAVE_bfd_elf32_sparc_vec \
		-DNETBSD_CORE


