#	$NetBSD: Makefile,v 1.1.2.2 2001/12/09 19:36:10 he Exp $

PROG=	mlxctl
SRCS=	cmds.c config.c dklist.c main.c util.c
MAN=	mlxctl.8

LDADD=	-lkvm
DPADD=	${LIBKVM}

.include <bsd.prog.mk>
