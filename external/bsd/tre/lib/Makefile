#	$NetBSD: Makefile,v 1.1 2011/11/05 22:39:13 christos Exp $

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

WARNS=	4

.include <bsd.lib.mk>
