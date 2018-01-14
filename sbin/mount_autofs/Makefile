#	$NetBSD: Makefile,v 1.1 2018/01/14 22:44:04 christos Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

NOMAN=
.include <bsd.own.mk>

PROG=	mount_autofs
SRCS=	mount_autofs.c
#MAN=	mount_autofs.8

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
