#	$NetBSD: Makefile,v 1.12 2002/08/19 10:17:01 lukem Exp $
#	@(#)Makefile	8.4 (Berkeley) 6/22/95

.include <bsd.own.mk>

PROG=	umount
SRCS=	umount.c vfslist.c
MAN=	umount.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
