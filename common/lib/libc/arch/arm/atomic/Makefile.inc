#	$NetBSD: Makefile.inc,v 1.7.8.2 2014/02/15 10:27:44 matt Exp $

ARMV6= ${CPUFLAGS:M-march=armv7*} ${CPUFLAGS:M-mcpu=cortex*}
ARMV6+= ${CPUFLAGS:M-march=armv6*} ${CPUFLAGS:M-mcpu=arm11*}
ARMV6+= ${CFLAGS:M-march=armv7*:} ${CFLAGS:M-mcpu=cortex*}
ARMV6+= ${CFLAGS:M-march=armv6*:} ${CFLAGS:M-mcpu=arm11*}
ARMV6+= ${CPPFLAGS:M-march=armv7*:} ${CPPFLAGS:M-mcpu=cortex*}
ARMV6+= ${CPPFLAGS:M-march=armv6*:} ${CPPFLAGS:M-mcpu=arm11*}


.if defined(LIB) && (${LIB} == "kern" || ${LIB} == "c" || ${LIB} == "pthread")
.if ${MACHINE_ARCH:Marmv7*} == "" && empty(ARMV6)

SRCS+=	atomic_add_32_cas.c atomic_add_32_nv_cas.c atomic_and_32_cas.c \
	atomic_and_32_nv_cas.c atomic_dec_32_cas.c atomic_dec_32_nv_cas.c \
	atomic_inc_32_cas.c atomic_inc_32_nv_cas.c atomic_or_32_cas.c \
	atomic_or_32_nv_cas.c atomic_swap_32_cas.c membar_ops_nop.c

.else
SRCS+=	atomic_add_32.S atomic_and_32.S atomic_cas_32.S
SRCS+=	atomic_dec_32.S atomic_inc_32.S atomic_or_32.S
SRCS+=	atomic_add_64.S atomic_and_64.S atomic_cas_64.S
SRCS+=	atomic_dec_64.S atomic_inc_64.S atomic_or_64.S
SRCS+=	atomic_swap.S membar_ops.S
.endif
.endif

.if defined(LIB) && (${LIB} == "c" || ${LIB} == "pthread")

.if ${MACHINE_ARCH:Marmv7*} == "" && empty(ARMV6)
SRCS+=	atomic_init_testset.c
SRCS+=	atomic_cas_up.S
CPPFLAGS+= -D__HAVE_ASM_ATOMIC_CAS_UP
.else
SRCS+=	atomic_init_cas.c

.endif

.endif
