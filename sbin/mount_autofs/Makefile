#	$NetBSD: Makefile,v 1.2 2018/01/22 09:45:32 kamil Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_autofs
SRCS=	mount_autofs.c
MAN=	mount_autofs.8

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
