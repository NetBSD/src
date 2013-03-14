#	$NetBSD: Makefile,v 1.5 2013/03/14 01:23:34 pooka Exp $
#

LIB=	rumpnet_virtif

SRCS=	if_virt.c
SRCS+=	component.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpkern

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
