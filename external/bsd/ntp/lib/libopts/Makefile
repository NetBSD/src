#	$NetBSD: Makefile,v 1.14.6.1 2018/04/11 02:58:47 msaitoh Exp $

LIBISPRIVATE=yes

LIB=opts

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/sntp/libopts

CPPFLAGS+=-I${DIST}

.PATH.c:	${DIST}

SRCS=libopts.c

COPTS.libopts.c += -DPOSIX_SHELL="\"/bin/sh\"" -Wno-format-nonliteral
COPTS.libopts.c+=	${${ACTIVE_CC} == "gcc":? -Wno-format-contains-nul :}
COPTS.libopts.c+=	${${ACTIVE_CC} == "clang":? -Wno-format :}

.include <bsd.lib.mk>
