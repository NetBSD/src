#	$NetBSD: Makefile,v 1.2 2011/05/24 13:48:16 joerg Exp $

.include "bsd.own.mk"

PROG=	yacc
SRCS=	closure.c error.c lalr.c lr0.c main.c mkpar.c output.c reader.c \
	skeleton.c symtab.c verbose.c warshall.c graph.c

CPPFLAGS+=	-DGCC_NORETURN=__dead

.include <bsd.prog.mk>
