#	$NetBSD: Makefile.inc,v 1.6.4.1 2008/05/18 12:28:45 yamt Exp $

.if defined(LIB) && (${LIB} == "kern" || ${LIB} == "c")

SRCS+=	atomic_add_32_cas.c atomic_add_32_nv_cas.c atomic_and_32_cas.c \
	atomic_and_32_nv_cas.c atomic_dec_32_cas.c atomic_dec_32_nv_cas.c \
	atomic_inc_32_cas.c atomic_inc_32_nv_cas.c atomic_or_32_cas.c \
	atomic_or_32_nv_cas.c atomic_swap_32_cas.c atomic_add_64_cas.c \
	atomic_add_64_nv_cas.c atomic_and_64_cas.c atomic_and_64_nv_cas.c \
	atomic_dec_64_cas.c atomic_dec_64_nv_cas.c atomic_inc_64_cas.c \
	atomic_inc_64_nv_cas.c atomic_or_64_cas.c atomic_or_64_nv_cas.c \
	atomic_swap_64_cas.c membar_ops.o

.endif

.if defined(LIB) && (${LIB} == "c")

SRCS+=	membar_ops_nop.c atomic_init_testset.c atomic_cas_up.S
CPPFLAGS+= -D__HAVE_ASM_ATOMIC_CAS_UP

.endif

.if defined(LIB) && (${LIB} == "kern")

SRCS+=	membar_ops.S

.endif
