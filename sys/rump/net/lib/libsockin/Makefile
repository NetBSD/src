#	$NetBSD: Makefile,v 1.5.22.1 2013/06/23 06:20:29 tls Exp $
#

LIB=	rumpnet_sockin

SRCS=	sockin.c
SRCS+=	component.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern

RUMPCOMP_USER=	#defined

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
