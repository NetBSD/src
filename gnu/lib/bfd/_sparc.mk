#	$NetBSD: _sparc.mk,v 1.5 1998/08/29 16:09:56 tv Exp $

BFD_ARCH_SRCS=	cpu-sparc.c \
		sparcnetbsd.c sunos.c aout32.c \
		elf32-sparc.c elf32.c \
		elf.c elflink.c \
		sparc-dis.c sparc-opc.c

BFD_ARCH_DEFS=	-DARCH_sparc \
		-DSELECT_ARCHITECTURES='&bfd_sparc_arch' \
		-DDEFAULT_VECTOR=sparcnetbsd_vec \
		-DSELECT_VECS='&sparcnetbsd_vec, &sunos_big_vec, &bfd_elf32_sparc_vec' \
		-DHAVE_sparcnetbsd_vec \
		-DHAVE_sunos_big_vec \
		-DHAVE_bfd_elf32_sparc_vec \
		-DNETBSD_CORE
