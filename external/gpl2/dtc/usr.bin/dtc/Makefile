#	$NetBSD: Makefile,v 1.4 2019/12/22 12:42:23 skrll Exp $

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
