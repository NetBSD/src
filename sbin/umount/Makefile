#	$NetBSD: Makefile,v 1.18 2013/07/01 15:16:33 christos Exp $
#	@(#)Makefile	8.4 (Berkeley) 6/22/95

.include <bsd.own.mk>

PROG=	umount
SRCS=	umount.c
MAN=	umount.8

.ifdef SMALLPROG
CPPFLAGS+= -DSMALL
.else
MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}
SRCS+=	vfslist.c
.endif

.include <bsd.prog.mk>
