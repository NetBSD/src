#	$NetBSD: Makefile,v 1.7 2003/03/22 12:44:00 jdolecek Exp $
#
#	Id: Makefile,v 1.1.1.1 1999/02/03 03:51:19 semenu Exp 
#

.include <bsd.own.mk>

PROG=	mount_ntfs
SRCS=	mount_ntfs.c fattr.c
MAN=	mount_ntfs.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
