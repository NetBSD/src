#	$NetBSD: Makefile,v 1.3 2012/02/01 07:46:30 kardel Exp $

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
