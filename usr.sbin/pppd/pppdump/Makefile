#	$NetBSD: Makefile,v 1.1 2002/05/29 19:07:13 christos Exp $

PROG=	pppdump
SRCS=	pppdump.c deflate.c bsd-comp.c
MAN=	pppdump.8

DPADD+=${LIBZ}
LDADD+=-lz
.include <bsd.prog.mk>
