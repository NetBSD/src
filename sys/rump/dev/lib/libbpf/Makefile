#	$NetBSD: Makefile,v 1.1.24.2 2017/12/03 11:39:06 jdolecek Exp $
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpdev_bpf
COMMENT=Berkeley Packet Filter

SRCS=	bpf.c bpf_filter.c
SRCS+=	bpf_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
