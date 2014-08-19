#	$NetBSD: Makefile,v 1.1.18.1 2014/08/20 00:04:38 tls Exp $
#

.PATH:	${.CURDIR}/../../../../dev/pud

LIB=	rumpdev_pud

SRCS=	pud.c pud_dev.c

SRCS+=	pud_component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
