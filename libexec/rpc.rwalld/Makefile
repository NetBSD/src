#	$NetBSD: Makefile,v 1.10 1997/10/08 09:07:33 mrg Exp $

PROG =	rpc.rwalld
SRCS =	rwalld.c
MAN =	rpc.rwalld.8
MLINKS=	rpc.rwalld.8 rwalld.8

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

.include <bsd.prog.mk>
