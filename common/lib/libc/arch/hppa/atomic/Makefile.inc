#	$NetBSD: Makefile.inc,v 1.11 2014/02/22 08:24:27 skrll Exp $

.if defined(LIB)

.  if (${LIB} == "kern" || ${LIB} == "c" || ${LIB} == "pthread" || \
    ${LIB} == "rump")

SRCS+=	atomic_add_32_cas.c atomic_add_32_nv_cas.c \
	atomic_and_32_cas.c atomic_and_32_nv_cas.c \
	atomic_dec_32_cas.c atomic_dec_32_nv_cas.c \
	atomic_inc_32_cas.c atomic_inc_32_nv_cas.c \
	atomic_or_32_cas.c atomic_or_32_nv_cas.c \
	atomic_swap_32_cas.c \
	membar_ops_nop.c

.  endif

.  if (${LIB} == "c" || ${LIB} == "pthread")

SRCS+=	atomic_init_testset.c
SRCS+=	atomic_cas_up.S
CPPFLAGS+= -D__HAVE_ASM_ATOMIC_CAS_UP
CPPFLAGS+= -D__HAVE_ASM_ATOMIC_CAS_16_UP
CPPFLAGS+= -D__HAVE_ASM_ATOMIC_CAS_8_UP
.  endif

.endif
