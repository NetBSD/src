#	$NetBSD: Makefile,v 1.4 2016/01/26 23:12:14 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpdev_bpf
COMMENT=Berkeley Packet Filter

SRCS=	bpf.c bpf_filter.c
SRCS+=	bpf_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
