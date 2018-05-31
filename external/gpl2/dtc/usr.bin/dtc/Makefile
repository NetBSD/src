#	$NetBSD: Makefile,v 1.3 2018/05/31 09:08:25 mrg Exp $

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
CLEANFILES+= dtc-parser.h

.PATH:	${DTCDIST}

.include <bsd.prog.mk>
