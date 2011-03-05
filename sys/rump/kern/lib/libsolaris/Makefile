#	$NetBSD: Makefile,v 1.1.4.2 2011/03/05 20:56:13 rmind Exp $
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
