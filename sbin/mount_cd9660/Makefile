#	$NetBSD: Makefile,v 1.10 2005/02/05 14:49:36 xtraeme Exp $
#	@(#)Makefile	8.3 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_cd9660
SRCS=	mount_cd9660.c
MAN=	mount_cd9660.8

WARNS=	3

MOUNT=	${NETBSDSRCDIR}/sbin/mount

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
