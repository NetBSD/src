#	from: @(#)Makefile	8.4 (Berkeley) 3/27/94
#	$Id: Makefile,v 1.4 1994/06/08 19:25:35 mycroft Exp $

PROG=	mount_procfs
SRCS=	mount_procfs.c getmntopts.c
MAN8=	mount_procfs.0

MOUNT=	${.CURDIR}/../mount
CFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
