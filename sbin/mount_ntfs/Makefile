#	$NetBSD: Makefile,v 1.5 2002/08/19 10:16:58 lukem Exp $
#
#	Id: Makefile,v 1.1.1.1 1999/02/03 03:51:19 semenu Exp 
#

.include <bsd.own.mk>

PROG=	mount_ntfs
SRCS=	mount_ntfs.c getmntopts.c fattr.c
MAN=	mount_ntfs.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
