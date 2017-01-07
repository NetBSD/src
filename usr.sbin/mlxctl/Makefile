#	$NetBSD: Makefile,v 1.4.6.1 2017/01/07 08:57:00 pgoyette Exp $

PROG=	mlxctl
SRCS=	cmds.c config.c dklist.c main.c util.c
MAN=	mlxctl.8

LDADD=	-lutil
DPADD=	${LIBUTIL}

.include <bsd.prog.mk>
