#	$NetBSD: Makefile,v 1.2 2004/11/11 19:10:16 jdolecek Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_ptyfs
SRCS=	mount_ptyfs.c
MAN=	mount_ptyfs.8

CPPFLAGS+= -I${NETBSDSRCDIR}/sys

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
