#	$NetBSD: Makefile.inc,v 1.3 2007/11/29 15:29:46 ad Exp $

.if defined(LIB) && (${LIB} == "kern")

SRCS+=	atomic_add_32_cas.c atomic_add_32_nv_cas.c atomic_and_32_cas.c \
	atomic_and_32_nv_cas.c atomic_dec_32_cas.c atomic_dec_32_nv_cas.c \
	atomic_inc_32_cas.c atomic_inc_32_nv_cas.c atomic_or_32_cas.c \
	atomic_or_32_nv_cas.c atomic_swap_32_cas.c atomic_add_64_cas.c \
	atomic_add_64_nv_cas.c atomic_and_64_cas.c atomic_and_64_nv_cas.c \
	atomic_dec_64_cas.c atomic_dec_64_nv_cas.c atomic_inc_64_cas.c \
	atomic_inc_64_nv_cas.c atomic_or_64_cas.c atomic_or_64_nv_cas.c \
	atomic_swap_64_cas.c

.endif
