#	$NetBSD: Makefile,v 1.3.4.1 2004/11/15 09:12:38 rtr Exp $

NOMAN=		1

.include <bsd.own.mk>

PROG=	Xprt

.include "../Makefile.Xserver"

LDADD+=	${LIBDIR.printer}/ddxInit.o \
	${LIBDIR.printer}/miinitext.o \
	${LIBDIR.printer}/dpmsstubs.o

LDADD+= ${LIBCWRAPPER} \
	${LDADD.dix} \
	${LDADD.os} \
	${LDADD.printer} \
	${LDADD.printer/raster} \
	${LDADD.printer/pcl} \
	${LDADD.printer/pcl-mono} \
	${LDADD.printer/ps} \
	${LDADD.mfb} \
	${LDADD.cfb} \
	${LDADD.cfb32} \
	${LDADD.mfb} \
	${LDADD.dix/xpstubs} \
	${LDADD.mi} \
	${LDADD.composite} \
	${LDADD.damage} \
	${LDADD.miext/damage} \
	${LDADD.xfixes} \
	${LDADD.miext/cw} \
	${LDADD.Xext/exts} \
	${LDADD.xkb} \
	${LDADD.xinput} \
	${LDADD.lbx} \
	-llbxutil \
	${LDADD.randr} \
	${LDADD.render} \
	${LDADD.Xext/ext} \
	${LDADD.dbe} \
	${LDADD.record} \
	${LDADD.GL/glx} \
	${LDADD.GL/GLcore} \
	${LDADD.xtrap} \
	-lXfont \
	-lfreetype \
	${LDADD.dix/xpstubs} \
	-lz \
	-lm \
	-lXau \
	-lXdmcp

#Xprint/ddxInit.o
#Xprint/miinitext.o
#Xprint/dpmsstubs.o
#os/libcwrapper.o
#dix/libdix.a
#os/libos.a
#Xprint/libprinter.a
#Xprint/raster/libraster.a
#Xprint/pcl/libpcl.a
#Xprint/pcl-mono/libpcl.a
#Xprint/ps/libps.a
#mfb/libmfb.a
#cfb/libcfb.a
#cfb32/libcfb32.a
#mfb/libmfb.a
#dix/libxpstubs.a
#mi/libmi.a
#composite/libcomposite.a
#damageext/libdamage.a
#miext/damage/libdamage.a
#xfixes/libxfixes.a
#miext/cw/libcw.a
#Xext/libexts.a
#xkb/libxkb.a
#Xi/libxinput.a
#lbx/liblbx.a
#../../lib/lbxutil/liblbxutil.a
#randr/librandr.a
#render/librender.a
#Xext/libext.a
#dbe/libdbe.a
#record/librecord.a
#GL/glx/libglx.a
#GL/mesa/GLcore/libGLcore.a
#XTrap/libxtrap.a -L/usr/X11R6/lib
#../../lib/font/libXfont.a
#-lfreetype
#dix/libxpstubs.a
#-lz
#-lm
#-lXau
#-lXdmcp
#-li386


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
	${LIBZ} ${LIBM}

.if ${MACHINE_ARCH} == "i386"
LDADD+=	-li386
DPADD+=	${LIBI386}
.endif

.include <bsd.x11.mk>
.include <bsd.prog.mk>
