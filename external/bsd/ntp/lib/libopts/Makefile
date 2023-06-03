#	$NetBSD: Makefile,v 1.17 2023/06/03 09:09:04 lukem Exp $

LIBISPRIVATE=yes

LIB=opts

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/sntp/libopts

CPPFLAGS+=-I${DIST}

.PATH.c:	${DIST}

SRCS=libopts.c

COPTS.libopts.c+=	 -DPOSIX_SHELL="\"/bin/sh\"" -Wno-format-nonliteral

COPTS.libopts.c+=	${${ACTIVE_CC} == "gcc":? -Wno-format-contains-nul :}
COPTS.libopts.c+=	${${ACTIVE_CC} == "clang":? -Wno-format :}
COPTS.libopts.c+=	${CC_WNO_FORMAT_OVERFLOW}

.include <bsd.lib.mk>
