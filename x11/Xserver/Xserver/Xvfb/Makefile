#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:05 lukem Exp $

.include <bsd.own.mk>

PROG=	Xvfb
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/vfb

.include "../Makefile.Xserver"

LDADD+=	${LDADD.dix} ${LDADD.os} -lXau -lXdmcp \
	${LDADD.hw/vfb} ${LDADD.fb} ${LDADD.mfb} ${LDADD.dix/xpstubs} \
	${LDADD.mi} ${LDADD.Xext/exts} ${LDADD.xkb} ${LDADD.xinput} \
	${LDADD.lbx} -llbxutil ${LDADD.randr} ${LDADD.render} \
	${LDADD.mi} ${LDADD.Xext/ext} ${LDADD.dbe} ${LDADD.record} \
	${LDADD.GL/glx} ${LDADD.GL/mesa} ${LDADD.xtrap} \
	-lXfont -lfreetype ${LDADD.dix/xpstubs} \
	-lz -lm -li386

DPADD+=	${DPADD.dix} ${DPADD.os} ${LIBXAU} ${LIBXDMCP} \
	${DPADD.hw/vfb} ${DPADD.fb} ${DPADD.mfb} ${DPADD.dix/xpstubs} \
	${DPADD.mi} ${DPADD.Xext/exts} ${DPADD.xkb} ${DPADD.xinput} \
	${DPADD.lbx} ${LIBLBXUTIL} ${DPADD.randr} ${DPADD.render} \
	${DPADD.mi} ${DPADD.Xext/ext} ${DPADD.dbe} ${DPADD.record} \
	${DPADD.GL/glx} ${DPADD.GL/mesa} ${DPADD.xtrap} \
	${LIBXFONT} ${LIBFREETYPE} ${DPADD.dix/xpstubs} \
	${LIBZ} ${LIBM} ${LIBI386}

.include <bsd.prog.mk>
.include <bsd.x11.mk>
