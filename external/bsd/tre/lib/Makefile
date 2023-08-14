#	$NetBSD: Makefile,v 1.6 2023/08/14 05:29:28 mrg Exp $

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

# XXXGCC12 this is very broken.  it double-free()'s as well as the UB
# after realloc().
COPTS.xmalloc.c+=	-Wno-error

.include <bsd.lib.mk>
