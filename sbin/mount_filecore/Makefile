#	$NetBSD: Makefile,v 1.2 1998/08/14 14:36:52 mark Exp $
#
#	Makefile	1.0	1998/6/26

PROG=	mount_filecore
SRCS=	mount_filecore.c getmntopts.c
MAN=	mount_filecore.8

MOUNT=	${.CURDIR}/../mount
CPPFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
