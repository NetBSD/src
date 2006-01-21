#	$NetBSD: Makefile,v 1.15 2006/01/21 10:34:32 dsl Exp $
#	@(#)Makefile	8.4 (Berkeley) 6/22/95

.include <bsd.own.mk>

PROG=	umount
SRCS=	umount.c vfslist.c
MAN=	umount.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

.ifdef SMALLPROG
CPPFLAGS+= -DSMALL
.endif

.include <bsd.prog.mk>
