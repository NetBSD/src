#	$NetBSD: Makefile,v 1.3.2.1 2020/04/08 14:04:22 martin Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	dtc

.include "${.CURDIR}/../../Makefile.inc"
.include "${DTCDIST}/Makefile.dtc"

CPPFLAGS+=	-I${.CURDIR}
CPPFLAGS+=	-I${DTCDIST}
CPPFLAGS+=	-I${LIBFDTDIST}
CPPFLAGS+=	-DNO_YAML

SRCS+=	${DTC_SRCS} dtc-lexer.l dtc-parser.y
YFLAGS+= -d -L
CLEANFILES+= dtc-parser.h

.PATH:	${DTCDIST}

.include <bsd.prog.mk>
