#	$NetBSD: Makefile,v 1.1.2.2 2009/08/19 18:48:26 yamt Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_cgd

SRCS=   cgd.c cgd_crypto.c

SRCS+=	component.c

CPPFLAGS+=	-Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
