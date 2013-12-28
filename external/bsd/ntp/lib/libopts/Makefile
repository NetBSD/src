#	$NetBSD: Makefile,v 1.6 2013/12/28 03:19:44 christos Exp $

LIBISPRIVATE=yes

LIB=opts

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/sntp/libopts

CPPFLAGS+=-I${DIST}

.PATH.c:	${DIST}

SRCS=libopts.c

COPTS.libopts.c += -Wno-format-contains-nul -Wno-format-nonliteral -Wno-error -DPOSIX_SHELL="\"/bin/sh\""
COPTS.find.c += -Wno-format-contains-nul
COPTS.makeshell.c += -Wno-format-contains-nul
COPTS.nested.c += -Wno-format-contains-nul
COPTS.pgusage.c += -Wno-format-contains-nul
COPTS.putshell.c += -Wno-format-contains-nul
COPTS.save.c += -Wno-format-contains-nul
COPTS.usage.c += -Wno-format-contains-nul

.include <bsd.lib.mk>
