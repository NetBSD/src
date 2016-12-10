#	$NetBSD: Makefile,v 1.5 2016/12/10 13:55:55 rin Exp $

PROG=	mlxctl
SRCS=	cmds.c config.c dklist.c main.c util.c
MAN=	mlxctl.8

LDADD=	-lutil
DPADD=	${LIBUTIL}

.include <bsd.prog.mk>
