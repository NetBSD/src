#	$NetBSD: Makefile,v 1.8 2002/08/19 10:16:59 lukem Exp $
#	@(#)Makefile	8.3 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_umap
SRCS=	mount_umap.c getmntopts.c
MAN=	mount_umap.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${NETBSDSRCDIR}/sys -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
