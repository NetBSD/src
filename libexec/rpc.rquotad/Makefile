#	$NetBSD: Makefile,v 1.5 1997/10/07 11:15:16 mrg Exp $

WARNS?=	1
PROG =	rpc.rquotad
SRCS =	rquotad.c
MAN  =	rpc.rquotad.8
MLINKS = rpc.rquotad.8 rquotad.8

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

.include <bsd.prog.mk>
