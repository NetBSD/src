#	$NetBSD: Makefile,v 1.10.2.2 2017/03/20 10:59:50 martin Exp $

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
