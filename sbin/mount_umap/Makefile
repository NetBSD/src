#	from: @(#)Makefile	8.3 (Berkeley) 3/27/94
#	$Id: Makefile,v 1.1 1994/06/08 19:28:10 mycroft Exp $

PROG=	mount_umap
SRCS=	mount_umap.c getmntopts.c
MAN8=	mount_umap.0

MOUNT=	${.CURDIR}/../mount
CFLAGS+= -I/sys -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
