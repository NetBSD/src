# $NetBSD: Makefile,v 1.1.58.1 2026/11/04 23:28:40 perseant Exp $

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
