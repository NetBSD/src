#	$NetBSD: Makefile.inc,v 1.1 2010/09/04 12:17:58 ahoka Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 6/4/93

# regex sources
TREDIST= ${NETBSDSRCDIR}/external/bsd/tre/dist

# Add libc/regex for manpages
.PATH: ${TREDIST}/lib ${NETBSDSRCDIR}/lib/libc/regex

CPPFLAGS+=	-I${TREDIST} -I${TREDIST}/lib
CPPFLAGS+=	-DHAVE_CONFIG_H=1

SRCS+=	regcomp.c regerror.c regexec.c
SRCS+=	tre-ast.c tre-compile.c
SRCS+=	tre-match-backtrack.c tre-match-parallel.c tre-mem.c
SRCS+=	tre-parse.c tre-stack.c

WARNS=	4

MAN+=	regex.3 re_format.7

MLINKS+=regex.3 regcomp.3 regex.3 regexec.3 regex.3 regerror.3 \
	regex.3 regfree.3
