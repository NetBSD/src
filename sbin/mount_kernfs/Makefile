#	from: @(#)Makefile	8.2 (Berkeley) 3/27/94
#	$Id: Makefile,v 1.4 1994/06/08 19:15:25 mycroft Exp $

PROG=	mount_kernfs
SRCS=	mount_kernfs.c getmntopts.c
MAN8=	mount_kernfs.0

MOUNT=	${.CURDIR}/../mount
CFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
