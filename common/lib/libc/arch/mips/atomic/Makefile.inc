#	$NetBSD: Makefile.inc,v 1.5 2008/02/10 18:45:40 ad Exp $

SRCS+=	atomic_add_32_cas.c atomic_add_32_nv_cas.c atomic_and_32_cas.c \
	atomic_and_32_nv_cas.c atomic_dec_32_cas.c atomic_dec_32_nv_cas.c \
	atomic_inc_32_cas.c atomic_inc_32_nv_cas.c atomic_or_32_cas.c \
	atomic_or_32_nv_cas.c atomic_swap_32_cas.c atomic_add_64_cas.c \
	atomic_add_64_nv_cas.c atomic_and_64_cas.c atomic_and_64_nv_cas.c \
	atomic_dec_64_cas.c atomic_dec_64_nv_cas.c atomic_inc_64_cas.c \
	atomic_inc_64_nv_cas.c atomic_or_64_cas.c atomic_or_64_nv_cas.c \
	atomic_swap_64_cas.c membar_ops.o

.if defined(LIB)
.if (${LIB} == "c")
SRCS+=	membar_ops_nop.c atomic_init_testset.c
.else
SRCS+=	membar_ops.S
.endif
.endif
