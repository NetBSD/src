#	$NetBSD: Makefile,v 1.1 2003/12/20 01:38:00 rtr Exp $

.include <bsd.own.mk>

PROG=	XsunMono
BINMODE=4711

MKMAN=	no

.include "../Makefile.Xserver"

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/sun

LDADD=	${LIBDIR.hw/sun}/sunInitMono.o ${LIBDIR.hw/sun}/sunInExMono.o \
	${LDADD.hw/sun} ${LDADD.dix} ${LDADD.os} ${LDADD.mfb} ${LDADD.mi} \
	${LDADD.Xext/ext} ${LDADD.xkb} ${LDADD.lbx} ${LDADD.lbxutil} \
	${LDADD.dbe} ${LDADD.record} ${LDADD.xtrap} ${LDADD.GL/glx} \
	${LDADD.GL/GLcore} ${LDADD.randr} ${LDADD.render} ${LDADD.Xfont} \
	-lfreetype ${LDADD.dix/xpstubs} -lfreetype -lXau -lXdmcp -lz -lm

DPADD=	${LIBDIR.hw/sun}/sunInitMono.o ${LIBDIR.hw/sun}/sunInExMono.o \
	${DPADD.hw/sun} ${DPADD.dix} ${DPADD.os} ${DPADD.mfb} ${DPADD.mi} \
	${LIBXEXT} ${DPADD.xkb} ${DPADD.lbx} ${DPADD.lbxutil} ${DPADD.dbe} \
	${DPADD.record} ${DPADD.xtrap} ${DPADD.GL/glx} ${DPADD.GL/GLcore} \
	${DPADD.randr} ${DPADD.render} ${LIBXFONT} ${DPADD.dix/xpstubs} \
	${LIBFREETYPE} ${LIBXAU} ${LIBXDMCP} ${LIBZ} ${LIBM}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
