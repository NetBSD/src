#	$NetBSD: Makefile.inc,v 1.4.2.2 2008/01/09 01:21:07 matt Exp $

.include <bsd.own.mk>

.if ${MACHINE} == "sparc64"

SPARC64DIR=	${.PARSEDIR}/../../sparc64/atomic
.PATH: ${SPARC64DIR}

.include "${SPARC64DIR}/Makefile.inc"

.else
. if defined(LIB) && (${LIB} == "kern")

SRCS+=	atomic_add_32_cas.c atomic_add_32_nv_cas.c atomic_and_32_cas.c \
	atomic_and_32_nv_cas.c atomic_dec_32_cas.c atomic_dec_32_nv_cas.c \
	atomic_inc_32_cas.c atomic_inc_32_nv_cas.c atomic_or_32_cas.c \
	atomic_or_32_nv_cas.c atomic_swap_32_cas.c \
	atomic_cas.S membar_ops.S

. endif

.endif
