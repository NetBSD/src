#	$NetBSD: Makefile,v 1.13.26.1 2009/05/13 19:20:46 jym Exp $

.include <bsd.own.mk>

PROG=	XFree86
BINMODE=4711

MAN=	XFree86.1 XF86Config.5

.PATH:  ${X11SRCDIR.xc}/programs/Xserver/hw/xfree86

.include "../Makefile.Xserver"

LDADD=	${LIBDIR.xkb}/xf86KillSrv.o ${LIBDIR.xkb}/xf86VT.o \
	${LIBDIR.xkb}/xf86Private.o ${LIBDIR.hw/xfree86/xf86}/xf86Init.o \
	${LIBDIR.hw/xfree86/xf86}/xf86IniExt.o ${LIBDIR.fontbase}/fontcache.o

LDADD+=	${LDADD.hw/xfree86/xf86} ${LDADD.hw/xfree86/xf86config} \
	${LDADD.hw/xfree86/xf86_os} ${LDADD.hw/xfree86/loader} \
	${LDADD.hw/xfree86/xf86} ${LDADD.dix} ${LDADD.os} -lXau -lXdmcp \
	${LDADD.fontbase} ${LDADD.Xext/exts} ${LDADD.xkb} \
	${LDADD.xinput} ${LDADD.lbx} -llbxutil \
	${LDADD.hw/xfree86/xf86} ${LDADD.Xext/exts} ${LDADD.xkb} \
	${LDADD.xinput} ${LDADD.lbx} -llbxutil ${LDADD.randr} \
	${LDADD.render} ${LDADD.dix/xpstubs} ${LDADD.mi} ${LDADD.Xext/exts} \
	${LDADD.xkb} ${LDADD.xinput} ${LDADD.lbx} -llbxutil \
	${LDADD.randr} ${LDADD.render} ${LDADD.hw/xfree86/xf86_os} \
	-lusbhid -lz -lm

DPADD=	${DPADD.hw/xfree86/xf86} ${DPADD.hw/xfree86/xf86config} \
	${DPADD.hw/xfree86/xf86_os} ${DPADD.hw/xfree86/loader} \
	${DPADD.dix} ${DPADD.os} ${LIBXAU} ${LIBXDMCP} ${DPADD.fontbase} \
	${DPADD.Xext/exts} ${DPADD.xkb} ${DPADD.xinput} ${DPADD.lbx} \
	${LIBLBXUTIL} ${DPADD.randr} ${DPADD.render} ${DPADD.dix/xpstubs} \
	${DPADD.mi} ${LIBZ} ${LIBM}

.if ${MACHINE_ARCH} == "x86_64"
LDADD+=	-lx86_64
DPADD+=	${LIBX86_64}
.elif ${MACHINE_ARCH} == "i386"
LDADD+=	-li386
DPADD+=	${LIBI386}
.endif

SYMLINKS=	${X11BINDIR}/XFree86 ${X11BINDIR}/X

.include <bsd.x11.mk>

.if (${MACHINE_ARCH} == "arm" || ${MACHINE_ARCH} == "armeb") \
        && defined(X11FLAGS.LOADABLE)
LDADD+= -larm
DPADD+= ${LIBARM}
.endif

LDADD+= -export-dynamic

.include <bsd.prog.mk>
