#	$NetBSD: Makefile,v 1.7 1997/10/10 19:51:28 christos Exp $
#	@(#)Makefile	8.3 (Berkeley) 3/27/94

PROG=	mount_umap
SRCS=	mount_umap.c getmntopts.c
MAN=	mount_umap.8

MOUNT=	${.CURDIR}/../mount
CPPFLAGS+= -I${.CURDIR}/../../sys -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
