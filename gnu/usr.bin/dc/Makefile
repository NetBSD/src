#	$NetBSD: Makefile,v 1.9 2002/09/19 03:09:35 lukem Exp $

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/gnu/dist

PROG=		dc
SRCS=		array.c dc.c eval.c misc.c numeric.c stack.c string.c \
		number.c
CPPFLAGS+=	-D_POSIX_SOURCE -I. -I${NETBSDSRCDIR}/gnu/usr.bin/bc \
		-I${DIST}/bc/dc -I${DIST}/bc/h

TEXINFO=	dc.texi

.PATH: ${DIST}/bc/dc ${DIST}/bc/lib ${DIST}/bc/doc

.include <bsd.info.mk>
.include <bsd.prog.mk>
