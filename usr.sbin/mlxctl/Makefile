#	$NetBSD: Makefile,v 1.1 2001/02/04 17:30:37 ad Exp $

PROG=	mlxctl
SRCS=	cmds.c config.c dklist.c main.c util.c
MAN=	mlxctl.8

LDADD=	-lkvm
DPADD=	${LIBKVM}

.include <bsd.prog.mk>
