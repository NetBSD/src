#	$NetBSD: Makefile,v 1.7 1999/02/11 16:22:44 tv Exp $

PROG=		dc
SRCS=		array.c dc.c eval.c misc.c numeric.c stack.c string.c \
		number.c getopt.c getopt1.c
CPPFLAGS+=	-D_POSIX_SOURCE -I. -I${.CURDIR}/../bc \
		-I${DIST}/bc/dc -I${DIST}/bc/h

TEXINFO=	dc.texi

DIST=		${.CURDIR}/../../dist
.PATH: ${DIST}/bc/dc ${DIST}/bc/lib ${DIST}/bc/doc

.include <bsd.info.mk>
.include <bsd.prog.mk>
