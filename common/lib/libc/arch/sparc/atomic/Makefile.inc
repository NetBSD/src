#	$NetBSD: Makefile.inc,v 1.6 2008/02/10 18:45:40 ad Exp $

.include <bsd.own.mk>

.if ${MACHINE} == "sparc64"

SPARC64DIR=	${.PARSEDIR}/../../sparc64/atomic
.PATH.c: ${SPARC64DIR}
.PATH.S: ${SPARC64DIR}

.include "${SPARC64DIR}/Makefile.inc"

.else

SRCS+=	atomic_add_32_cas.c atomic_add_32_nv_cas.c atomic_and_32_cas.c \
	atomic_and_32_nv_cas.c atomic_dec_32_cas.c atomic_dec_32_nv_cas.c \
	atomic_inc_32_cas.c atomic_inc_32_nv_cas.c atomic_or_32_cas.c \
	atomic_or_32_nv_cas.c atomic_swap_32_cas.c \
	atomic_cas.S membar_ops.S

.if defined(LIB) && (${LIB} == "c")

SRCS+=	atomic_init_cas.c

.endif

.endif
