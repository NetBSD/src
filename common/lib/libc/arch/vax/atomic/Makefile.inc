#	$NetBSD: Makefile.inc,v 1.5.14.1 2014/08/19 23:45:14 tls Exp $

.if defined(LIB) && (${LIB} == "kern" || ${LIB} == "c" || ${LIB} == "pthread" \
	|| ${LIB} == "rump")

SRCS+=	atomic_add_32_cas.c atomic_add_32_nv_cas.c atomic_and_32_cas.c \
	atomic_and_32_nv_cas.c atomic_dec_32_cas.c atomic_dec_32_nv_cas.c \
	atomic_inc_32_cas.c atomic_inc_32_nv_cas.c atomic_or_32_cas.c \
	atomic_or_32_nv_cas.c atomic_swap_32_cas.c membar_ops_nop.c

.if ${LIB} == "c"
SRCS+=	atomic_add_16_cas.c atomic_add_8_cas.c	\
	atomic_sub_32_cas.c atomic_sub_16_cas.c atomic_sub_8_cas.c	\
	atomic_or_16_cas.c atomic_or_8_cas.c	\
	atomic_xor_32_cas.c atomic_xor_16_cas.c atomic_xor_8_cas.c	\
	atomic_and_16_cas.c atomic_and_8_cas.c	\
	atomic_nand_32_cas.c atomic_nand_16_cas.c atomic_nand_8_cas.c	\
	atomic_cas_32_cas.c atomic_cas_16_cas.c atomic_cas_8_cas.c	\
	atomic_swap_16_cas.c atomic_swap_8_cas.c
.endif

.endif

.if defined(LIB) && (${LIB} == "c" || ${LIB} == "pthread")

SRCS+=	atomic_init_testset.c

.endif
