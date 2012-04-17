#	$NetBSD: Makefile,v 1.2.2.1 2012/04/17 00:03:53 yamt Exp $

LIBISPRIVATE=yes

LIB=opts

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/sntp/libopts

CPPFLAGS+=-I${DIST}

.PATH.c:	${DIST}

SRCS=libopts.c

COPTS.libopts.c += -Wno-format-nonliteral -Wno-error -DPOSIX_SHELL="\"/bin/sh\""

.include <bsd.lib.mk>
