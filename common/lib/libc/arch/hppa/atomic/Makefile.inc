#	$NetBSD: Makefile.inc,v 1.17 2026/01/09 08:44:57 skrll Exp $

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
CPPFLAGS+= -D__HAVE_ATOMIC_CAS_64_UP -D__HAVE_ASM_ATOMIC_CAS_64_UP
.  endif

.if defined(LIB) && ${LIB} == "c"
SRCS+=	atomic_xor_64_cas.c atomic_xor_32_cas.c atomic_xor_16_cas.c atomic_xor_8_cas.c		\
	atomic_sub_64_cas.c atomic_sub_32_cas.c atomic_sub_16_cas.c atomic_sub_8_cas.c		\
	atomic_nand_64_cas.c atomic_nand_32_cas.c atomic_nand_16_cas.c atomic_nand_8_cas.c	\
	atomic_or_64_cas.c atomic_or_16_cas.c atomic_or_8_cas.c					\
	atomic_or_64_nv_cas.c 									\
	atomic_and_64_cas.c atomic_and_16_cas.c atomic_and_8_cas.c				\
	atomic_and_64_nv_cas.c atomic_and_16_nv_cas.c atomic_and_8_nv_cas.c			\
	atomic_add_64_cas.c atomic_add_16_cas.c atomic_add_8_cas.c				\
	atomic_add_64_nv_cas.c 									\
	atomic_swap_64_cas.c atomic_swap_16_cas.c atomic_swap_8_cas.c				\
	atomic_cas_64_cas.c atomic_cas_32_cas.c atomic_cas_16_cas.c atomic_cas_8_cas.c		\
	atomic_dec_64_cas.c atomic_dec_64_nv_cas.c						\
	atomic_inc_64_cas.c atomic_inc_64_nv_cas.c						\
	atomic_c11_compare_exchange_cas_32.c							\
	atomic_c11_compare_exchange_cas_16.c							\
	atomic_c11_compare_exchange_cas_8.c							\
	atomic_is_lock_free.c


.endif
.endif
