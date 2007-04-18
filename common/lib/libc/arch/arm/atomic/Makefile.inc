#	$NetBSD: Makefile.inc,v 1.1.2.1 2007/04/18 07:32:30 thorpej Exp $

.if defined(LIB) && (${LIB} == "kern")

SRCS+=	atomic_add_8_cas.c atomic_add_8_nv_cas.c \
	atomic_add_16_cas.c atomic_add_16_nv_cas.c \
	atomic_add_32_cas.c atomic_add_32_nv_cas.c

SRCS+=	atomic_and_8_cas.c atomic_and_8_nv_cas.c \
	atomic_and_16_cas.c atomic_and_16_nv_cas.c \
	atomic_and_32_cas.c atomic_and_32_nv_cas.c

SRCS+=	atomic_cas_32.S
SRCS+=	atomic_cas_8_cas.c atomic_cas_16_cas.c

SRCS+=	atomic_dec_8_add.c atomic_dec_8_nv_add.c \
	atomic_dec_16_add.c atomic_dec_16_nv_add.c \
	atomic_dec_32_add.c atomic_dec_32_nv_add.c

SRCS+=	atomic_inc_8_add.c atomic_inc_8_nv_add.c \
	atomic_inc_16_add.c atomic_inc_16_nv_add.c \
	atomic_inc_32_add.c atomic_inc_32_nv_add.c

SRCS+=	atomic_or_8_cas.c atomic_or_8_nv_cas.c \
	atomic_or_16_cas.c atomic_or_16_nv_cas.c \
	atomic_or_32_cas.c atomic_or_32_nv_cas.c

SRCS+=	atomic_swap_8_cas.c atomic_swap_16_cas.c atomic_swap_32_cas.c

SRCS+=	membar_ops_nop.c

.endif
