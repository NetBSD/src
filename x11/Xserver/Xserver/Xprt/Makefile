#	$NetBSD: Makefile,v 1.2 2003/09/23 23:21:08 lukem Exp $

NOMAN=		1

.include <bsd.own.mk>

PROG=	Xprt

.include "../Makefile.Xserver"

LDADD+=	${LIBDIR.printer}/ddxInit.o ${LIBDIR.printer}/miinitext.o \
	${LIBDIR.printer}/dpmsstubs.o \
	${LDADD.dix} ${LDADD.os} -lXau -lXdmcp \
	${LDADD.printer} ${LDADD.printer/raster} ${LDADD.printer/pcl} \
	${LDADD.printer/ps} ${LDADD.mfb} ${LDADD.cfb} ${LDADD.cfb32} \
	${LDADD.mfb} ${LDADD.dix/xpstubs} ${LDADD.mi} ${LDADD.Xext/exts} \
	${LDADD.xkb} ${LDADD.xinput} ${LDADD.lbx} -llbxutil \
	${LDADD.randr} ${LDADD.render} ${LDADD.Xext/ext} ${LDADD.dbe} \
	${LDADD.record} ${LDADD.GL/glx} ${LDADD.GL/GLcore} ${LDADD.xtrap} \
	${LIBCWRAPPER} \
	-lXfont -lfreetype ${LDADD.dix/xpstubs} \
	-lz -lm -li386

DPADD+=	${LIBDIR.printer}/ddxInit.o ${LIBDIR.printer}/miinitext.o \
	${LIBDIR.printer}/dpmsstubs.o \
	${DPADD.dix} ${DPADD.os} ${LIBXAU} ${LIBXDMCP} \
	${DPADD.printer} ${DPADD.printer/raster} ${DPADD.printer/pcl} \
	${DPADD.printer/ps} ${DPADD.mfb} ${DPADD.cfb} ${DPADD.cfb32} \
	${DPADD.mfb} ${DPADD.dix/xpstubs} ${DPADD.mi} ${DPADD.Xext/exts} \
	${DPADD.xkb} ${DPADD.xinput} ${DPADD.lbx} ${LIBLBXUTIL} \
	${DPADD.randr} ${DPADD.render} ${DPADD.Xext/ext} ${DPADD.dbe} \
	${DPADD.record} ${DPADD.GL/glx} ${DPADD.GL/GLcore} ${DPADD.xtrap} \
	${LIBCWRAPPER} \
	${LIBXFONT} ${LIBFREETYPE} ${DPADD.dix/xpstubs} \
	${LIBZ} ${LIBM} ${LIBI386}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
