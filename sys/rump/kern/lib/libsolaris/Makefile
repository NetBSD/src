#	$NetBSD: Makefile,v 1.1 2011/01/18 22:21:23 haad Exp $
#

S!=             cd ${.PARSEDIR}/../../../../;pwd

.include "${.CURDIR}/../../../../modules/solaris/Makefile.solmod"

LIB=	rumpkern_solaris

# RUMP Stuff
CPPFLAGS+=      -I${RUMPTOP}/librump/rumpkern -I${RUMPTOP}/librump/rumpvfs\
		-I${RUMPTOP}/librump/rumpdev
CPPFLAGS+=	-DASSERT=KASSERT

SRCS+= component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
