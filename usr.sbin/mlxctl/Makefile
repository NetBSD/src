#	$NetBSD: Makefile,v 1.2 2011/06/22 02:49:45 mrg Exp $

PROG=	mlxctl
SRCS=	cmds.c config.c dklist.c main.c util.c
MAN=	mlxctl.8

LDADD=	-lkvm
DPADD=	${LIBKVM}

.include <bsd.prog.mk>

# XXX
.if ${HAVE_GCC} == 45
COPTS.mlxctl.c+=	-fno-strict-aliasing
COPTS.cmds.c+=	-fno-strict-aliasing
.endif
