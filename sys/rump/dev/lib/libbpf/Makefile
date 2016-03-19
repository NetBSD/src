#	$NetBSD: Makefile,v 1.2.6.2 2016/03/19 11:30:34 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpdev_bpf
COMMENT=Berkeley Packet Filter

SRCS=	bpf.c bpf_filter.c
SRCS+=	bpf_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
