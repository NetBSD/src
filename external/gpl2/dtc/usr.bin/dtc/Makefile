#	$NetBSD: Makefile,v 1.1 2017/06/05 18:59:43 christos Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	dtc

.include "${.CURDIR}/../../Makefile.inc"
.include "${DIST}/Makefile.dtc"

CPPFLAGS+=	-I${.CURDIR}
CPPFLAGS+=	-I${DIST}
CPPFLAGS+=	-I${DIST}/libfdt

SRCS+=	${DTC_SRCS} dtc-lexer.l dtc-parser.y
YFLAGS+= -d -L

.PATH:	${DIST}

.include <bsd.prog.mk>
