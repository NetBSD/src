#	from: @(#)Makefile	8.2 (Berkeley) 3/27/94
#	$Id: Makefile,v 1.1 1994/06/08 19:23:02 mycroft Exp $

PROG=	mount_nfs
SRCS=	mount_nfs.c getmntopts.c
MAN8=	mount_nfs.0

MOUNT=	${.CURDIR}/../mount
CFLAGS+= -DNFS -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
