#	$NetBSD: Makefile.inc,v 1.4 2008/02/10 18:45:40 ad Exp $

SRCS+=	atomic_add_64_cas.c atomic_add_64_nv_cas.c atomic_and_64_cas.c \
	atomic_and_64_nv_cas.c atomic_dec_64_cas.c atomic_dec_64_nv_cas.c \
	atomic_inc_64_cas.c atomic_inc_64_nv_cas.c atomic_or_64_cas.c \
	atomic_or_64_nv_cas.c atomic_swap_64_cas.c atomic.S

.if defined(LIB) && (${LIB} == "c")

SRCS+=	atomic_init_cas.c

.endif
