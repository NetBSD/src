#	$NetBSD: Makefile,v 1.10 2003/03/22 12:44:00 jdolecek Exp $
#	@(#)Makefile	8.3 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_null
SRCS=	mount_null.c
MAN=	mount_null.8

CPPFLAGS+= -I${NETBSDSRCDIR}/sys

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
