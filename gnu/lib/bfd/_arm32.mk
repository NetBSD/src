# $NetBSD: _arm32.mk,v 1.1 1997/10/17 19:58:55 gwr Exp $

BFD_MACHINES =	cpu-arm.c
BFD_BACKENDS =	aout-arm.c aout32.c

ARCH_DEFS = -DARCH_arm \
 -DSELECT_ARCHITECTURES='&bfd_arm_arch'

TDEFAULTS = \
 -DDEFAULT_VECTOR=aout_arm_little_vec \
 -DSELECT_VECS=' &aout_arm_little_vec, &aout_arm_big_vec ' \
 -DHAVE_aout_arm_little_vec \
 -DHAVE_aout_arm_big_vec \
 -DNETBSD_CORE

OPCODE_MACHINES =  arm-dis.c
