#	$NetBSD: Makefile,v 1.5.16.2 2017/05/04 06:01:09 snj Exp $

LIBISPRIVATE=yes

LIB=opts

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/sntp/libopts

CPPFLAGS+=-I${DIST}

.PATH.c:	${DIST}

SRCS=libopts.c

COPTS.libopts.c += -DPOSIX_SHELL="\"/bin/sh\"" -Wno-format-nonliteral
.if ${HAVE_GCC:U} == 4
COPTS.libopts.c+=	-Wno-error
.else
COPTS.libopts.c+=	${${ACTIVE_CC} == "gcc":? -Wno-format-contains-nul :}
.endif
COPTS.libopts.c+=	${${ACTIVE_CC} == "clang":? -Wno-format :}

.include <bsd.lib.mk>
