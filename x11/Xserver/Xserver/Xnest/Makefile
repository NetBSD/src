#	$NetBSD: Makefile,v 1.2 2003/09/23 23:21:07 lukem Exp $

.include <bsd.own.mk>

PROG=	Xnest

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xnest

.include "../Makefile.Xserver"

LDADD+=	${LDADD.dix} ${LDADD.os} -lXau -lXdmcp ${LDADD.hw/xnest} \
	${LDADD.Xext/exts} ${LDADD.xkb} ${LDADD.xinput} ${LDADD.lbx} \
	-llbxutil ${LDADD.randr} ${LDADD.render} ${LDADD.dix/xpstubs} \
	${LDADD.mi} ${LDADD.Xext/exts} ${LDADD.xkb} ${LDADD.xinput} \
	${LDADD.lbx} -llbxutil ${LDADD.randr} ${LDADD.render} \
	${LDADD.hw/xnest} ${LDADD.Xext/ext} ${LDADD.dbe} ${LDADD.record} \
	${LDADD.GL/glx} ${LDADD.GL/GLcore} ${LDADD.xtrap} ${LIBCWRAPPER} \
	-lXfont -lfreetype ${LDADD.dix/xpstubs} \
	-lXext -lX11 -lz -lm -li386

DPADD+=	${DPADD.dix} ${DPADD.os} ${LIBXAU} ${LIBXDMCP} ${DPADD.hw/xnest} \
	${DPADD.Xext/exts} ${DPADD.xkb} ${DPADD.xinput} ${DPADD.lbx} \
	${LIBLBXUTIL} ${DPADD.randr} ${DPADD.render} ${DPADD.dix/xpstubs} \
	${DPADD.mi} ${DPADD.Xext/exts} ${DPADD.xkb} ${DPADD.xinput} \
	${DPADD.lbx} ${LIBLBXUTIL} ${DPADD.randr} ${DPADD.render} \
	${DPADD.hw/xnest} ${DPADD.Xext/ext} ${DPADD.dbe} ${DPADD.record} \
	${DPADD.GL/glx} ${DPADD.GL/GLcore} ${DPADD.xtrap} ${LIBCWRAPPER} \
	${LIBXFONT} ${LIBFREETYPE} ${DPADD.dix/xpstubs} \
	${LIBXEXT} ${LIBX11} ${LIBZ} ${LIBM} ${LIBI386}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
