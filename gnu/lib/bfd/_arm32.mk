#	$NetBSD: _arm32.mk,v 1.3 1998/08/22 19:02:10 tv Exp $

BFD_ARCH_SRCS=	cpu-arm.c \
		armnetbsd.c aoutarm32.c \
		arm-dis.c

BFD_ARCH_DEFS=	-DARCH_arm \
		-DSELECT_ARCHITECTURES='&bfd_arm_arch' \
		-DDEFAULT_VECTOR=armnetbsd_vec \
		-DSELECT_VECS='&armnetbsd_vec' \
		-DHAVE_armnetbsd_vec \
		-DNETBSD_CORE
