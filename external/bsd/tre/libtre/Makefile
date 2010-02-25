#	$NetBSD: Makefile,v 1.1 2010/02/25 07:46:14 agc Exp $

# for information:
# the configure script is run as:
#	./configure --prefix=/usr --without-alloca

TREDIST=	${.CURDIR}/../dist

# external tre sources
.PATH: ${TREDIST}/lib

CPPFLAGS+=	-I${TREDIST} -I${TREDIST}/lib -I${.CURDIR}
CPPFLAGS+=	-DHAVE_CONFIG_H=1

LIB=	tre

SRCS+=	regcomp.c regerror.c regexec.c
SRCS+=	tre-ast.c tre-compile.c tre-match-approx.c
SRCS+=	tre-match-backtrack.c tre-match-parallel.c tre-mem.c
SRCS+=	tre-parse.c tre-stack.c xmalloc.c

WARNS=	4

.include <bsd.lib.mk>
