#	$NetBSD: Makefile,v 1.12 2014/12/23 20:19:08 christos Exp $

LIBISPRIVATE=yes

LIB=opts

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/sntp/libopts

CPPFLAGS+=-I${DIST}

.PATH.c:	${DIST}

SRCS=libopts.c

COPTS.libopts.c += -DPOSIX_SHELL="\"/bin/sh\"" -Wno-format-nonliteral
.if ${HAVE_GCC} != 4
COPTS.libopts.c+=	${${ACTIVE_CC} == "gcc":? -Wno-format-contains-nul :}
.else
COPTS.libopts.c+=	-Wno-error
.endif

.include <bsd.lib.mk>
