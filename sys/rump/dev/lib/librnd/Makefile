#	$NetBSD: Makefile,v 1.4 2011/12/19 21:56:18 apb Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_rnd

SRCS=   rnd.c rndpseudo.c rndpool.c
SRCS+=  ${.CURDIR}/../../../../compat/common/rndpseudo_50.c

SRCS+=	component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
