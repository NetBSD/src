#	$NetBSD: Makefile,v 1.2.10.1 2012/04/17 00:08:48 yamt Exp $
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
