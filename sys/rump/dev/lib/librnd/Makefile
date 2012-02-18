#	$NetBSD: Makefile,v 1.2.14.1 2012/02/18 07:35:45 mrg Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_rnd

SRCS=	rndpseudo.c

SRCS+=	component.c

# sys/rump/Makefile.rump sets CPPFLAGS+= -DCOMPAT_50=1,
# so we need rndpseudo_50.c from the compat/common directory.
.PATH:	${.CURDIR}/../../../../compat/common
SRCS+=	rndpseudo_50.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
