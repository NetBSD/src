#	$NetBSD: Makefile,v 1.9 1997/09/17 20:19:43 christos Exp $

WARNS ?= 1
PROG =	rpc.rwalld
SRCS =	rwalld.c
MAN =	rpc.rwalld.8
MLINKS=	rpc.rwalld.8 rwalld.8

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

.include <bsd.prog.mk>
