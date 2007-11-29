#	$NetBSD: Makefile.inc,v 1.2 2007/11/29 01:04:57 ad Exp $

.if defined(LIB) && (${LIB} == "kern")

SRCS+=	atomic_add_32_cas.c atomic_add_32_nv_cas.c atomic_and_32_cas.c \
	atomic_and_32_nv_cas.c atomic_dec_32_add.c atomic_dec_32_nv_add.c \
	atomic_inc_32_add.c atomic_inc_32_nv_add.c atomic_or_32_cas.c \
	atomic_or_32_nv_cas.c atomic_swap_32_cas.c atomic_add_64_cas.c \
	atomic_add_64_nv_cas.c atomic_and_64_cas.c atomic_and_64_nv_cas.c \
	atomic_dec_64_add.c atomic_dec_64_nv_add.c atomic_inc_64_add.c \
	atomic_inc_64_nv_add.c atomic_or_64_cas.c atomic_or_64_nv_cas.c \
	atomic_swap_64_cas.c

.endif
