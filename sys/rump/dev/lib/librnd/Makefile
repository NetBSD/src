#	$NetBSD: Makefile,v 1.5 2011/12/20 17:09:04 apb Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_rnd

SRCS=	rnd.c rndpseudo.c rndpool.c

SRCS+=	component.c

# sys/rump/Makefile.rump sets CPPFLAGS+= -DCOMPAT_50=1,
# so we need rndpseudo_50.c from the compat/common directory.
.PATH:	${.CURDIR}/../../../../compat/common
SRCS+=	rndpseudo_50.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
