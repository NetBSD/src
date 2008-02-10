#	$NetBSD: Makefile.inc,v 1.4 2008/02/10 18:45:40 ad Exp $

SRCS+=	atomic_add_32_cas.c atomic_add_32_nv_cas.c atomic_and_32_cas.c \
	atomic_and_32_nv_cas.c atomic_dec_32_cas.c atomic_dec_32_nv_cas.c \
	atomic_inc_32_cas.c atomic_inc_32_nv_cas.c atomic_or_32_cas.c \
	atomic_or_32_nv_cas.c atomic_swap_32_cas.c membar_ops_nop.c

.if defined(LIB) && (${LIB} == "c")

SRCS+=	atomic_init_testset.c

.endif
