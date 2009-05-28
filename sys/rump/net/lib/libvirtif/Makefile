#	$NetBSD: Makefile,v 1.3 2009/05/28 00:02:17 pooka Exp $
#

LIB=	rumpnet_virtif

SRCS=	if_virt.c
SRCS+=	component.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpkern

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
