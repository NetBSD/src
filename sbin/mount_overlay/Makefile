#	$NetBSD: Makefile,v 1.3 2003/03/22 12:44:01 jdolecek Exp $
#	@(#)Makefile	8.3 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_overlay
SRCS=	mount_overlay.c
MAN=	mount_overlay.8

CPPFLAGS+= -I${NETBSDSRCDIR}/sys

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
