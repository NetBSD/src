#	from: @(#)Makefile	8.2 (Berkeley) 3/27/94
#	$Id: Makefile,v 1.4 1994/06/08 19:11:36 mycroft Exp $

PROG=	mount_fdesc
SRCS=	mount_fdesc.c getmntopts.c
MAN8=	mount_fdesc.0

MOUNT=	${.CURDIR}/../mount
CFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
