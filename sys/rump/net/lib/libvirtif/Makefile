#	$NetBSD: Makefile,v 1.2.12.1 2009/07/23 23:32:56 jym Exp $
#

LIB=	rumpnet_virtif

SRCS=	if_virt.c
SRCS+=	component.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpkern

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
