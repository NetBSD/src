#	$NetBSD: Makefile.inc,v 1.16 2013/05/02 03:56:42 matt Exp $

SRCS+=	byte_swap_2.S byte_swap_4.S
SRCS+=	ffs.S
SRCS+=	divsi3.S udivsi3.S divide.S clzsi2.S modsi3.S umodsi3.S
SRCS+=	memcmp.S memcpy.S memset.S memmove.S strcmp.S strncmp.S

.if !empty(MACHINE_ARCH:Mearm*)
SRCS+=	__aeabi_ldivmod.S __aeabi_uldivmod.S __aeabi_lcmp.c __aeabi_ulcmp.c
.endif
