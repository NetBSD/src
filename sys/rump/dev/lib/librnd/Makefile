#	$NetBSD: Makefile,v 1.6 2012/02/02 19:43:08 tls Exp $
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
