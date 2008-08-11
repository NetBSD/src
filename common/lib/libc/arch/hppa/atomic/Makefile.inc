#	$NetBSD: Makefile.inc,v 1.4 2008/08/11 21:54:51 skrll Exp $

.if defined(LIB) && (${LIB} == "kern" || ${LIB} == "c")

SRCS+=	atomic_add_32_cas.c atomic_add_32_nv_cas.c atomic_and_32_cas.c \
	atomic_and_32_nv_cas.c atomic_dec_32_cas.c atomic_dec_32_nv_cas.c \
	atomic_inc_32_cas.c atomic_inc_32_nv_cas.c atomic_or_32_cas.c \
	atomic_or_32_nv_cas.c atomic_swap_32_cas.c membar_ops_nop.c

.endif

.if defined(LIB) && (${LIB} == "c")

SRCS+=	atomic_init_testset.c
SRCS+=	atomic_cas_up.S
CPPFLAGS+= -D__HAVE_ASM_ATOMIC_CAS_UP

.endif
