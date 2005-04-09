#	$NetBSD: Makefile,v 1.5 2005/04/09 18:00:01 tron Exp $

.include <bsd.own.mk>

PROG=	Xvfb
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/vfb

.include "../Makefile.Xserver"

LDADD+=	${LDADD.dix} ${LDADD.hw/vfb} ${LDADD.os} -lXau -lXdmcp \
	${LDADD.fb} ${LDADD.mfb} ${LDADD.dix/xpstubs} \
	${LDADD.mi} ${LDADD.Xext/exts} ${LDADD.xkb} ${LDADD.xinput} \
	${LDADD.lbx} -llbxutil ${LDADD.randr} ${LDADD.render} \
	${LDADD.mi} ${LDADD.Xext/ext} ${LDADD.dbe} ${LDADD.record} \
	${LDADD.GL/glx} ${LDADD.GL/GLcore} ${LDADD.xtrap} \
	${LIBCWRAPPER} \
	-lXfont -lfreetype ${LDADD.dix/xpstubs} \
	-lz -lm

DPADD+=	${DPADD.dix} ${DPADD.os} ${LIBXAU} ${LIBXDMCP} \
	${DPADD.hw/vfb} ${DPADD.fb} ${DPADD.mfb} ${DPADD.dix/xpstubs} \
	${DPADD.mi} ${DPADD.Xext/exts} ${DPADD.xkb} ${DPADD.xinput} \
	${DPADD.lbx} ${LIBLBXUTIL} ${DPADD.randr} ${DPADD.render} \
	${DPADD.mi} ${DPADD.Xext/ext} ${DPADD.dbe} ${DPADD.record} \
	${DPADD.GL/glx} ${DPADD.GL/GLcore} ${DPADD.xtrap} \
	${LIBCWRAPPER} \
	${LIBXFONT} ${LIBFREETYPE} ${DPADD.dix/xpstubs} \
	${LIBZ} ${LIBM}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
