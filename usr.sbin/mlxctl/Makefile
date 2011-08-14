#	$NetBSD: Makefile,v 1.3 2011/08/14 17:57:44 christos Exp $

PROG=	mlxctl
SRCS=	cmds.c config.c dklist.c main.c util.c
MAN=	mlxctl.8

LDADD=	-lkvm
DPADD=	${LIBKVM}

.include <bsd.prog.mk>
