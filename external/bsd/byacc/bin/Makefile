#	$NetBSD: Makefile,v 1.4 2015/01/03 23:22:52 christos Exp $

.include "bsd.own.mk"

PROG=	yacc
SRCS=	closure.c error.c lalr.c lr0.c main.c mkpar.c output.c reader.c \
	symtab.c verbose.c warshall.c graph.c mstring.c btyaccpar.c

CPPFLAGS+=	-DGCC_NORETURN=__dead

COPTS.output.c += -Wno-format-nonliteral
COPTS.reader.c += -Wno-format-nonliteral

.include <bsd.prog.mk>
