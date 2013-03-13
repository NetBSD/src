#	$NetBSD: Makefile,v 1.4 2013/03/13 21:13:45 pooka Exp $
#

LIB=	rumpnet_virtif

SRCS=	if_virt.c
SRCS+=	component.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpkern -I${.CURDIR}

RUMPCOMP_USER=	#defined

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
