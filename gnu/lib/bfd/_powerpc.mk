#	$NetBSD: _powerpc.mk,v 1.4 1999/02/02 22:16:58 tv Exp $

BFD_ARCH_SRCS=	cpu-powerpc.c \
		elf32-ppc.c elf32.c elf.c elflink.c dwarf2.c \
		ppc-dis.c ppc-opc.c

BFD_ARCH_DEFS=	-DARCH_powerpc \
		-DSELECT_ARCHITECTURES='&bfd_powerpc_arch' \
		-DDEFAULT_VECTOR=bfd_elf32_powerpc_vec \
		-DSELECT_VECS='&bfd_elf32_powerpc_vec, &bfd_elf32_powerpcle_vec' \
		-DHAVE_bfd_elf32_powerpc_vec \
		-DHAVE_bfd_elf32_powerpcle_vec \
		-DNETBSD_CORE
