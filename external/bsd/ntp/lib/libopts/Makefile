#	$NetBSD: Makefile,v 1.4 2012/02/01 20:49:09 kardel Exp $

LIBISPRIVATE=yes

LIB=opts

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

# libopts is not really great code
WARNS=0

DIST=	${IDIST}/sntp/libopts

CPPFLAGS+=-I${DIST}

.PATH.c:	${DIST}

SRCS=libopts.c

COPTS.libopts.c += -Wno-format-nonliteral -Wno-error -DPOSIX_SHELL="\"/bin/sh\""

.include <bsd.lib.mk>
