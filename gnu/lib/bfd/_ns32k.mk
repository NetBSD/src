# $NetBSD: _ns32k.mk,v 1.4 1998/08/22 19:02:10 tv Exp $

BFD_ARCH_SRCS=	ns32knetbsd.c aout-ns32k.c \
		cpu-ns32k.c \
		ns32k-dis.c

BFD_ARCH_DEFS=	-DARCH_ns32k \
		-DSELECT_ARCHITECTURES='&bfd_ns32k_arch' \
		-DDEFAULT_VECTOR=pc532netbsd_vec \
		-DSELECT_VECS='&pc532netbsd_vec' \
		-DHAVE_pc532netbsd_vec \
		-DNETBSD_CORE
