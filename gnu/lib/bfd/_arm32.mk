# $NetBSD: _arm32.mk,v 1.2 1998/05/19 19:59:58 tv Exp $

BFD_MACHINES =	cpu-arm.c
BFD_BACKENDS =	armnetbsd.c aoutarm32.c

ARCH_DEFS = -DARCH_arm \
 -DSELECT_ARCHITECTURES='&bfd_arm_arch'

TDEFAULTS = \
 -DDEFAULT_VECTOR=armnetbsd_vec \
 -DSELECT_VECS=' &armnetbsd_vec ' \
 -DHAVE_armnetbsd_vec \
 -DNETBSD_CORE

OPCODE_MACHINES =  arm-dis.c
