#	$NetBSD: Makefile,v 1.2.2.2 2014/05/22 15:50:17 yamt Exp $

LIBISPRIVATE=yes

LIB=opts

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/sntp/libopts

CPPFLAGS+=-I${DIST}

.PATH.c:	${DIST}

SRCS=libopts.c

COPTS.libopts.c += -DPOSIX_SHELL="\"/bin/sh\"" -Wno-format-nonliteral
.if ${HAVE_GCC:U} >= 45
COPTS.libopts.c+=	${${ACTIVE_CC} == "gcc":? -Wno-format-contains-nul :}
.else
COPTS.libopts.c+=	-Wno-format
.endif

.include <bsd.lib.mk>
