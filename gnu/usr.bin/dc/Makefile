#	$NetBSD: Makefile,v 1.8 2001/01/11 01:49:47 phil Exp $

PROG=		dc
SRCS=		array.c dc.c eval.c misc.c numeric.c stack.c string.c \
		number.c
CPPFLAGS+=	-D_POSIX_SOURCE -I. -I${.CURDIR}/../bc \
		-I${DIST}/bc/dc -I${DIST}/bc/h

TEXINFO=	dc.texi

DIST=		${.CURDIR}/../../dist
.PATH: ${DIST}/bc/dc ${DIST}/bc/lib ${DIST}/bc/doc

.include <bsd.info.mk>
.include <bsd.prog.mk>
