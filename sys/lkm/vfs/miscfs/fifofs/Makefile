#	$NetBSD: Makefile,v 1.3 1996/08/27 21:44:43 explorer Exp $

.include "../Makefile.inc"

.PATH:	$s/miscfs/fifofs

NOMAN=	yep

#
# fifo needs this.  Can this module even be loaded?
CFLAGS+=	-DFIFO

LKM=	fifofs

SRCS=	lkminit_vfs.c
SRCS+=	fifo_vnops.c

.include <bsd.lkm.mk>
