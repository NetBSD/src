#	$NetBSD: Makefile,v 1.2 1999/05/06 16:08:54 christos Exp $
#
#	Id: Makefile,v 1.1.1.1 1999/02/03 03:51:19 semenu Exp 
#

PROG=	mount_ntfs
SRCS=	mount_ntfs.c getmntopts.c
MAN=	mount_ntfs.8

MOUNT=	${.CURDIR}/../mount
CPPFLAGS+= -I${.CURDIR}/../../sys -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
