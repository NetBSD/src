#	$NetBSD: Makefile,v 1.12 2003/03/22 12:44:02 jdolecek Exp $
#	@(#)Makefile	8.4 (Berkeley) 7/13/94

.include <bsd.own.mk>

PROG=	mount_union
SRCS=	mount_union.c
MAN=	mount_union.8

CPPFLAGS+= -I${NETBSDSRCDIR}/sys

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
