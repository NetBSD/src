#	$NetBSD: Makefile,v 1.7 2013/12/29 03:32:00 christos Exp $

LIBISPRIVATE=yes

LIB=opts

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/sntp/libopts

CPPFLAGS+=-I${DIST}

.PATH.c:	${DIST}

SRCS=libopts.c

COPTS.libopts.c += -Wno-format-nonliteral -Wno-error -DPOSIX_SHELL="\"/bin/sh\""

.if defined(HAVE_GCC) && ${HAVE_GCC} >= 45
COPTS.libopts.c += -Wno-format-contains-nul
COPTS.find.c += -Wno-format-contains-nul
COPTS.makeshell.c += -Wno-format-contains-nul
COPTS.nested.c += -Wno-format-contains-nul
COPTS.pgusage.c += -Wno-format-contains-nul
COPTS.putshell.c += -Wno-format-contains-nul
COPTS.save.c += -Wno-format-contains-nul
COPTS.usage.c += -Wno-format-contains-nul
.endif

.include <bsd.lib.mk>
