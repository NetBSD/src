#	$NetBSD: _i386.mk,v 1.4 1998/08/22 19:02:10 tv Exp $

BFD_ARCH_SRCS=	cpu-i386.c \
		i386netbsd.c aout32.c \
		elf32-i386.c elf32.c elf.c elflink.c \
		i386-dis.c

BFD_ARCH_DEFS=	-DARCH_i386 \
		-DSELECT_ARCHITECTURES='&bfd_i386_arch' \
		-DDEFAULT_VECTOR=i386netbsd_vec \
		-DSELECT_VECS='&i386netbsd_vec, &bfd_elf32_i386_vec' \
		-DHAVE_i386netbsd_vec \
		-DHAVE_bfd_elf32_i386_vec \
		-DNETBSD_CORE
