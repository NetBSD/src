#	$NetBSD: Makefile,v 1.10 2005/02/05 15:14:25 xtraeme Exp $
#	@(#)Makefile	8.3 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_umap
SRCS=	mount_umap.c
MAN=	mount_umap.8

WARNS=	3

CPPFLAGS+= -I${NETBSDSRCDIR}/sys

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
