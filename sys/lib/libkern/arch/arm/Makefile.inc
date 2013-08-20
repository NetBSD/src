#	$NetBSD: Makefile.inc,v 1.20 2013/08/20 21:43:03 matt Exp $

SRCS+=	byte_swap_2.S byte_swap_4.S
SRCS+=	ffs.S
SRCS+=	divsi3.S udivsi3.S divide.S clzsi2.S
SRCS+=	memcmp.S memcpy.S memset.S memmove.S strcmp.S strncmp.S
SRCS+=	modsi3.S umodsi3.S

.if !empty(MACHINE_ARCH:Mearm*)
SRCS+=	__aeabi_ldivmod.S __aeabi_uldivmod.S __aeabi_lcmp.c __aeabi_ulcmp.c
SRCS+=	unwind_stub.c
.endif
.if empty(MACHINE_ARCH:Mearmv7*)
CPUFLAGS.ffs.S+=	-marm
.endif
CPUFLAGS.divide.S+=	-marm
CPUFLAGS.memcmp.S+=	-marm
CPUFLAGS.memcpy.S+=	-marm
CPUFLAGS.memmove.S+=	-marm
CPUFLAGS.memset.S+=	-marm
.if empty(CPPFLAGS:M-D_STANDALONE)
CPUFLAGS.strcpy.S+=	-marm
CPUFLAGS.strlcpy.S+=	-marm
CPUFLAGS.strncpy.S+=	-marm
.endif
