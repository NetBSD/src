#	$NetBSD: Makefile.inc,v 1.1 2007/11/29 00:16:23 ad Exp $

.if defined(LIB) && (${LIB} == "kern")

SRCS+=	atomic_add_32_cas.c atomic_add_32_nv_cas.c atomic_and_32_cas.c \
	atomic_and_32_nv_cas.c atomic_dec_32_add.c atomic_dec_32_nv_add.c \
	atomic_inc_32_add.c atomic_inc_32_nv_add.c atomic_or_32_cas.c \
	atomic_or_32_nv_cas.c atomic_swap_32_cas.c

.endif
