#	$NetBSD: Makefile,v 1.13 2005/01/19 20:43:06 xtraeme Exp $
#	@(#)Makefile	8.4 (Berkeley) 6/22/95

.include <bsd.own.mk>

WARNS=	2
PROG=	umount
SRCS=	umount.c vfslist.c
MAN=	umount.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
