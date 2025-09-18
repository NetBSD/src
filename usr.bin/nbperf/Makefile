# $NetBSD: Makefile,v 1.2 2025/09/18 03:36:01 mrg Exp $

PROG=	nbperf
SRCS=	nbperf.c
SRCS+=	nbperf-bdz.c nbperf-chm.c nbperf-chm3.c
SRCS+=	graph2.c graph3.c

COPTS.graph2.c+=	${CC_WNO_CALLOC_TRANSPOSED_ARGS}
COPTS.graph3.c+=	${CC_WNO_CALLOC_TRANSPOSED_ARGS}
COPTS.nbperf-bdz.c+=	${CC_WNO_CALLOC_TRANSPOSED_ARGS}
COPTS.nbperf-chm.c+=	${CC_WNO_CALLOC_TRANSPOSED_ARGS}
COPTS.nbperf-chm3.c+=	${CC_WNO_CALLOC_TRANSPOSED_ARGS}

.include <bsd.prog.mk>
