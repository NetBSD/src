#	from: @(#)Makefile	8.3 (Berkeley) 3/27/94
#	$Id: Makefile,v 1.3 1994/12/22 10:46:39 cgd Exp $

PROG=	mount_umap
SRCS=	mount_umap.c getmntopts.c
MAN=	mount_umap.8

MOUNT=	${.CURDIR}/../mount
CFLAGS+= -I${.CURDIR}/../../sys -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
