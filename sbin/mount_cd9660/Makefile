#	$NetBSD: Makefile,v 1.11 2005/06/27 01:00:05 christos Exp $
#	@(#)Makefile	8.3 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_cd9660
SRCS=	mount_cd9660.c
MAN=	mount_cd9660.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
