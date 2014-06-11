#	$NetBSD: Makefile,v 1.4 2014/06/11 14:51:49 joerg Exp $

PROG=	mlxctl
SRCS=	cmds.c config.c dklist.c main.c util.c
MAN=	mlxctl.8

LDADD=	-lutil
DPADD=	${LIButil}

.include <bsd.prog.mk>
