#	$NetBSD: Makefile,v 1.4 2000/10/30 20:57:00 jdolecek Exp $
#
#	Id: Makefile,v 1.1.1.1 1999/02/03 03:51:19 semenu Exp 
#

PROG=	mount_ntfs
SRCS=	mount_ntfs.c getmntopts.c fattr.c
MAN=	mount_ntfs.8

MOUNT=	${.CURDIR}/../mount
CPPFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
