#	$NetBSD: Makefile,v 1.1.14.1 2014/05/22 11:41:11 yamt Exp $
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpdev_bpf

SRCS=	bpf.c bpf_filter.c
SRCS+=	bpf_component.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
