#	$NetBSD: Makefile,v 1.1 2010/01/19 22:38:21 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpdev_bpf

SRCS=	bpf.c bpf_filter.c
SRCS+=	component.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
