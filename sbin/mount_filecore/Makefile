#	$NetBSD: Makefile,v 1.3 2000/10/30 20:56:59 jdolecek Exp $
#
#	Makefile	1.0	1998/6/26

PROG=	mount_filecore
SRCS=	mount_filecore.c getmntopts.c fattr.c
MAN=	mount_filecore.8

MOUNT=	${.CURDIR}/../mount
CPPFLAGS+= -I${MOUNT}
.PATH: ${MOUNT}

.include <bsd.prog.mk>
