#	$NetBSD: Makefile,v 1.9.2.1 2014/08/10 07:09:33 tls Exp $

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

.include <bsd.lib.mk>
