#	from: @(#)Makefile	8.3 (Berkeley) 3/27/94
#	$Id: Makefile,v 1.2 1994/08/03 21:07:18 deraadt Exp $

PROG=	mount_umap
SRCS=	mount_umap.c getmntopts.c
MAN8=	mount_umap.0

MOUNT=	${.CURDIR}/../mount
CFLAGS+= -I${.CURDIR}/../../sys -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
