#	$NetBSD: Makefile,v 1.13 2005/02/05 15:14:25 xtraeme Exp $
#	@(#)Makefile	8.4 (Berkeley) 7/13/94

.include <bsd.own.mk>

PROG=	mount_union
SRCS=	mount_union.c
MAN=	mount_union.8

WARNS=	3

CPPFLAGS+= -I${NETBSDSRCDIR}/sys

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
