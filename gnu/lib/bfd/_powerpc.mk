#	$NetBSD: _powerpc.mk,v 1.3 1998/08/22 19:02:10 tv Exp $

BFD_ARCH_SRCS=	cpu-powerpc.c \
		elf32-ppc.c elf32.c elf.c elflink.c \
		ppc-dis.c ppc-opc.c

BFD_ARCH_DEFS=	-DARCH_powerpc \
		-DSELECT_ARCHITECTURES='&bfd_powerpc_arch' \
		-DDEFAULT_VECTOR=bfd_elf32_powerpc_vec \
		-DSELECT_VECS='&bfd_elf32_powerpc_vec, &bfd_elf32_powerpcle_vec' \
		-DHAVE_bfd_elf32_powerpc_vec \
		-DHAVE_bfd_elf32_powerpcle_vec \
		-DNETBSD_CORE
