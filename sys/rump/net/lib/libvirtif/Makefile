#	$NetBSD: Makefile,v 1.6.6.1 2013/08/28 23:59:37 rmind Exp $
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
