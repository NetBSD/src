#	$NetBSD: _vax.mk,v 1.5 1998/10/29 17:31:11 matt Exp $

BFD_ARCH_SRCS=	cpu-vax.c \
		vaxnetbsd.c aout32.c \
		vax-dis.c

BFD_ARCH_DEFS=	-DARCH_vax \
		-DSELECT_ARCHITECTURES='&bfd_vax_arch' \
		-DDEFAULT_VECTOR=vaxnetbsd_vec \
		-DSELECT_VECS='&vaxnetbsd_vec' \
		-DHAVE_vaxnetbsd_vec \
		-DNETBSD_CORE
