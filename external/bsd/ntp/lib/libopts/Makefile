#	$NetBSD: Makefile,v 1.10.2.1 2014/12/24 00:05:28 riz Exp $

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

.include <bsd.lib.mk>
