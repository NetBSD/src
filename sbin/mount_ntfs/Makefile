#	$NetBSD: Makefile,v 1.8 2005/02/05 15:06:15 xtraeme Exp $
#
#	Id: Makefile,v 1.1.1.1 1999/02/03 03:51:19 semenu Exp 
#

.include <bsd.own.mk>

PROG=	mount_ntfs
SRCS=	mount_ntfs.c fattr.c
MAN=	mount_ntfs.8

WARNS=	3

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
