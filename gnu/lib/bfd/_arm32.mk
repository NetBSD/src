#	$NetBSD: _arm32.mk,v 1.4 1999/02/02 20:31:07 tv Exp $

BFD_ARCH_SRCS=	cpu-arm.c \
		aout-arm32.c armnetbsd.c \
		arm-dis.c

BFD_ARCH_DEFS=	-DARCH_arm \
		-DSELECT_ARCHITECTURES='&bfd_arm_arch' \
		-DDEFAULT_VECTOR=armnetbsd_vec \
		-DSELECT_VECS='&armnetbsd_vec' \
		-DHAVE_armnetbsd_vec \
		-DNETBSD_CORE
