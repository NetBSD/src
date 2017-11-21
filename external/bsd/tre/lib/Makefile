#	$NetBSD: Makefile,v 1.3 2017/11/21 03:09:41 rin Exp $

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

.include <bsd.lib.mk>
