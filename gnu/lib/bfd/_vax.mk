#	$NetBSD: _vax.mk,v 1.4 1998/08/22 19:02:11 tv Exp $

BFD_ARCH_SRCS=	cpu-vax.c \
		vaxnetbsd.c aout32.c

BFD_ARCH_DEFS=	-DARCH_vax \
		-DSELECT_ARCHITECTURES='&bfd_vax_arch' \
		-DDEFAULT_VECTOR=vaxnetbsd_vec \
		-DSELECT_VECS='&vaxnetbsd_vec' \
		-DHAVE_vaxnetbsd_vec \
		-DNETBSD_CORE
