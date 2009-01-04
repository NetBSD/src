#	$NetBSD: Makefile.inc,v 1.11 2009/01/04 17:54:29 pooka Exp $

.include <bsd.own.mk>

.if ${MACHINE} == "sparc64"

SPARC64DIR=	${.PARSEDIR}/../../sparc64/atomic
.PATH.c: ${SPARC64DIR}
.PATH.S: ${SPARC64DIR}

.include "${SPARC64DIR}/Makefile.inc"

.else

. if defined(LIB)

.  if (${LIB} == "kern" || ${LIB} == "c"  || ${LIB} == "pthread"	\
    || ${LIB} == "rump")
SRCS+=	atomic_add_32_cas.c atomic_add_32_nv_cas.c atomic_and_32_cas.c \
	atomic_and_32_nv_cas.c atomic_dec_32_cas.c atomic_dec_32_nv_cas.c \
	atomic_inc_32_cas.c atomic_inc_32_nv_cas.c atomic_or_32_cas.c \
	atomic_or_32_nv_cas.c atomic_swap_32_cas.c membar_ops.S
.  endif

.  if (${LIB} == "kern")
SRCS+=	atomic_cas.S
.  endif

.  if (${LIB} == "c" || ${LIB} == "pthread")
SRCS+=	atomic_init_testset.c
.  endif

. endif

.endif
