#	from: @(#)Makefile	8.2 (Berkeley) 3/27/94
#	$Id: Makefile,v 1.2 1994/12/22 10:46:06 cgd Exp $

PROG=	mount_lfs
SRCS=	mount_lfs.c getmntopts.c
MAN=	mount_lfs.8

MOUNT=	${.CURDIR}/../mount
CFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
