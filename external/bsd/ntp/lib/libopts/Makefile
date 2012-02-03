#	$NetBSD: Makefile,v 1.5 2012/02/03 16:33:10 christos Exp $

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
