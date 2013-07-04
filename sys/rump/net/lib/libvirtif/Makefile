#	$NetBSD: Makefile,v 1.7 2013/07/04 11:46:51 pooka Exp $
#

LIB=	rumpnet_virtif

SRCS=	if_virt.c
SRCS+=	component.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpkern -I${.CURDIR}
CPPFLAGS+=	-DVIRTIF_BASE=virt

RUMPCOMP_USER=	#defined
RUMPCOMP_USER_CPPFLAGS+= -DVIRTIF_BASE=virt

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
