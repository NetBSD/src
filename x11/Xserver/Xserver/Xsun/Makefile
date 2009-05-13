#	$NetBSD: Makefile,v 1.9.26.1 2009/05/13 19:20:46 jym Exp $

.include <bsd.own.mk>

PROG=	Xsun
BINMODE=4711

.include "../../Makefile.common"

MAN=		Xsun.1

MLINKS=		Xsun.1 XsunMono.1
.if ${XSERVER_XSUN24} != "no"
MLINKS+=	Xsun.1 Xsun24.1
.endif

.include "../Makefile.Xserver"

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/sun

LDADD=	${LIBDIR.hw/sun}/sunInit.o ${LDADD.dix} ${LDADD.hw/sun} ${LDADD.os} \
	${LDADD.cfb} ${LDADD.mfb} ${LDADD.dix/xpstubs} ${LDADD.mi} \
	${LDADD.Xext/ext} ${LDADD.xkb}

.if ${MACHINE_ARCH} == "sparc64" || ${MACHINE_ARCH} == "sparc"
LDADD+=	${LDADD.xinput}
DPADD=	${DPADD.xinput}
.endif

LDADD+=	${LDADD.lbx} -llbxutil ${LDADD.dbe} ${LDADD.record} \
	${LDADD.xtrap} ${LDADD.GL/glx} ${LDADD.GL/GLcore} ${LDADD.randr} \
	${LDADD.render} -lXfont -lfreetype ${LDADD.dix/xpstubs} \
	-lfreetype -lXau -lXdmcp -lz -lm

DPADD+=	${LIBDIR.hw/sun}/sunInit.o ${DPADD.hw/sun} ${DPADD.dix} ${DPADD.os} \
	${DPADD.cfb} ${DPADD.mfb} ${DPADD.mi} ${LIBXEXT} ${DPADD.xkb} \
	${DPADD.lbx} ${LIBLBXUTIL} ${DPADD.dbe} ${DPADD.record} \
	${DPADD.xtrap} ${DPADD.GL/glx} ${DPADD.GL/GLcore} ${DPADD.randr} \
	${DPADD.render} ${LIBXFONT} ${LDADD.dix/xpstubs} ${LIBFREETYPE} \
	${LIBXAU} ${LIBXDMCP} ${LIBZ} ${LIBM}

.if ${MACHINE_ARCH} != "sparc64"
# sparc64 uses XFree86 as default X link
SYMLINKS=	${X11BINDIR}/Xsun ${X11BINDIR}/X
.endif

.include <bsd.x11.mk>
.include <bsd.prog.mk>
