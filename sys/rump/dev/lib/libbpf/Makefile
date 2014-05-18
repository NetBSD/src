#	$NetBSD: Makefile,v 1.1.28.1 2014/05/18 17:46:15 rmind Exp $
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpdev_bpf

SRCS=	bpf.c bpf_filter.c
SRCS+=	bpf_component.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
