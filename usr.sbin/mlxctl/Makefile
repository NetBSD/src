#	$NetBSD: Makefile,v 1.3.8.1 2014/08/20 00:05:10 tls Exp $

PROG=	mlxctl
SRCS=	cmds.c config.c dklist.c main.c util.c
MAN=	mlxctl.8

LDADD=	-lutil
DPADD=	${LIButil}

.include <bsd.prog.mk>
