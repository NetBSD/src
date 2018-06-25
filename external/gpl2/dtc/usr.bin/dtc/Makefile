#	$NetBSD: Makefile,v 1.2.6.1 2018/06/25 07:25:27 pgoyette Exp $

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
