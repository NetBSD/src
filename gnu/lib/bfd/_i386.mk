#	$NetBSD: _i386.mk,v 1.6 1999/02/02 22:16:57 tv Exp $

BFD_ARCH_SRCS=	cpu-i386.c \
		i386bsd.c i386freebsd.c i386netbsd.c aout32.c \
		coff-i386.c cofflink.c \
		elf32-i386.c elf32.c elf.c elflink.c dwarf2.c \
		i386-dis.c

BFD_ARCH_DEFS=	-DARCH_i386 \
		-DSELECT_ARCHITECTURES='&bfd_i386_arch' \
		-DDEFAULT_VECTOR=i386netbsd_vec \
		-DSELECT_VECS='&i386netbsd_vec, &bfd_elf32_i386_vec, &i386bsd_vec, &i386freebsd_vec, &i386coff_vec' \
		-DHAVE_i386netbsd_vec \
		-DHAVE_bfd_elf32_i386_vec \
		-DHAVE_i386bsd_vec \
		-DHAVE_i386freebsd_vec \
		-DHAVE_i386coff_vec \
		-DNETBSD_CORE
