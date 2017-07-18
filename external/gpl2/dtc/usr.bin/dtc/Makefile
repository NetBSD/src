#	$NetBSD: Makefile,v 1.2.4.2 2017/07/18 17:17:53 snj Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	dtc

.include "${.CURDIR}/../../Makefile.inc"
.include "${DTCDIST}/Makefile.dtc"

CPPFLAGS+=	-I${.CURDIR}
CPPFLAGS+=	-I${DTCDIST}
CPPFLAGS+=	-I${LIBFDTDIST}

SRCS+=	${DTC_SRCS} dtc-lexer.l dtc-parser.y
YFLAGS+= -d -L

.PATH:	${DTCDIST}

.include <bsd.prog.mk>
