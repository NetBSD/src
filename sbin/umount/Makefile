#	$NetBSD: Makefile,v 1.17 2013/06/29 22:53:04 christos Exp $
#	@(#)Makefile	8.4 (Berkeley) 6/22/95

.include <bsd.own.mk>

PROG=	umount
SRCS=	umount.c
MAN=	umount.8

.ifdef SMALLPROG
CPPFLAGS+= -DSMALL
.else
MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${MOUNT} -I${MOUNT}_nfs
.PATH:	${MOUNT} ${MOUNT}_nfs
SRCS+=	vfslist.c getnfsargs.c
.endif

.include <bsd.prog.mk>
