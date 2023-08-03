#	$NetBSD: Makefile,v 1.4 2023/08/03 13:36:08 rin Exp $

# for information:
# the configure script is run as:
#	./configure --prefix=/usr --without-alloca

.include <bsd.own.mk>
TREDIST=	${.CURDIR}/../dist

# external tre sources
.PATH: ${TREDIST}/lib

CPPFLAGS+=	-I${.CURDIR}

LIB=	tre

SRCS+=	regcomp.c regerror.c regexec.c
SRCS+=	tre-ast.c tre-compile.c tre-match-approx.c
SRCS+=	tre-match-backtrack.c tre-match-parallel.c tre-mem.c
SRCS+=	tre-parse.c tre-stack.c xmalloc.c

COPTS.xmalloc.c+=	${CC_WNO_USE_AFTER_FREE}

.include <bsd.lib.mk>
