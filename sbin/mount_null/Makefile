#	$NetBSD: Makefile,v 1.12 2005/06/27 01:00:06 christos Exp $
#	@(#)Makefile	8.3 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_null
SRCS=	mount_null.c
MAN=	mount_null.8

CPPFLAGS+= -I${NETBSDSRCDIR}/sys

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
