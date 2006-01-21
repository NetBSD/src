#	$NetBSD: Makefile,v 1.16 2006/01/21 11:59:53 dsl Exp $
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
