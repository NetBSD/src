#	from: @(#)Makefile	8.4 (Berkeley) 3/27/94
#	$Id: Makefile,v 1.5 1994/12/22 10:46:30 cgd Exp $

PROG=	mount_procfs
SRCS=	mount_procfs.c getmntopts.c
MAN=	mount_procfs.8

MOUNT=	${.CURDIR}/../mount
CFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
