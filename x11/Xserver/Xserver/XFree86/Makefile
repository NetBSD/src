#	$NetBSD: Makefile,v 1.12.2.1 2004/11/15 09:12:38 rtr Exp $

.include <bsd.own.mk>

PROG=	XFree86
BINMODE=4711

MAN=	Xorg.1 xorg.conf.5

.PATH:  ${X11SRCDIR.xc}/programs/Xserver/hw/xfree86

.include "../Makefile.Xserver"

.if ${X11LOADABLE} != "no"

LDADD=  ${LIBDIR.xkb}/xf86KillSrv.o \
	${LIBDIR.xkb}/xf86VT.o \
        ${LIBDIR.xkb}/xf86Private.o \
	${LIBDIR.hw/xfree86/xf86}/xf86Init.o \
        ${LIBDIR.hw/xfree86/xf86}/xf86IniExt.o \
	${LIBDIR.fontbase}/fontcache.o

LDADD+= ${LDADD.hw/xfree86/xf86} \
	${LDADD.hw/xfree86/xf86config} \
        ${LDADD.hw/xfree86/xf86_os} \
	${LDADD.hw/xfree86/loader} \
        ${LDADD.hw/xfree86/xf86} \
	${LDADD.dix} \
	${LDADD.os} \
	-lXau \
	-lXdmcp \
        ${LDADD.fontbase} \
	${LDADD.Xext/exts} \
	${LDADD.xkb} \
        ${LDADD.xinput} \
	${LDADD.lbx} \
	-llbxutil \
        ${LDADD.hw/xfree86/xf86} \
	${LDADD.Xext/exts} \
	${LDADD.xkb} \
        ${LDADD.xinput} \
	${LDADD.lbx} \
	-llbxutil \
	${LDADD.randr} \
        ${LDADD.render} \
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
	${LDADD.hw/xfree86/xf86_os} \
        -lz \
	-lm

DPADD=  ${DPADD.hw/xfree86/xf86} ${DPADD.hw/xfree86/xf86config} \
        ${DPADD.hw/xfree86/xf86_os} ${DPADD.hw/xfree86/loader} \
        ${DPADD.dix} ${DPADD.os} ${LIBXAU} ${LIBXDMCP} ${DPADD.fontbase} \
        ${DPADD.Xext/exts} ${DPADD.xkb} ${DPADD.xinput} ${DPADD.lbx} \
        ${LIBLBXUTIL} ${DPADD.randr} ${DPADD.render} ${DPADD.dix/xpstubs} \
        ${DPADD.mi} ${LIBZ} ${LIBM}

.else # X11LOADABLE


LDADD=	${LIBDIR.xkb}/xf86KillSrv.o \
	${LIBDIR.xkb}/xf86VT.o \
	${LIBDIR.xkb}/xf86Private.o \
	${LIBDIR.hw/xfree86/xf86}/xf86Init.o \
	${LIBDIR.hw/xfree86/xf86}/xf86IniExt.o \
	${LIBDIR.fontbase}/fontcache.o

LDADD+=	${LDADD.hw/xfree86/xf86} \
	${LDADD.hw/xfree86/xf86config} \
	${LDADD.hw/xfree86/xf86_os} \
	${LDADD.hw/xfree86/loader} \
	${LDADD.hw/xfree86/xf86} \
	${LDADD.dix} \
	${LDADD.os} \
	-lXau \
	-lXdmcp \
	${LDADD.fontbase} \
	${LDADD.Xext/exts} \
	${LDADD.xkb} \
	${LDADD.xinput} \
	${LDADD.lbx} \
	-llbxutil \
	${LDADD.hw/xfree86/xf86} \
	${LDADD.Xext/exts} \
	${LDADD.xkb} \
	${LDADD.xinput} \
	${LDADD.lbx} \
	-llbxutil \
	${LDADD.randr} \
	${LDADD.render} \
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
	${LDADD.hw/xfree86/xf86_os} \
	-lz \
	-lm

DPADD=	${DPADD.hw/xfree86/xf86} ${DPADD.hw/xfree86/xf86config} \
	${DPADD.hw/xfree86/xf86_os} ${DPADD.hw/xfree86/loader} \
	${DPADD.dix} ${DPADD.os} ${LIBXAU} ${LIBXDMCP} ${DPADD.fontbase} \
	${DPADD.Xext/exts} ${DPADD.xkb} ${DPADD.xinput} ${DPADD.lbx} \
	${LIBLBXUTIL} ${DPADD.randr} ${DPADD.render} ${DPADD.dix/xpstubs} \
	${DPADD.mi} ${LIBZ} ${LIBM}

.endif # X11LOADABLE

.if ${MACHINE_ARCH} == "x86_64"
LDADD+=	-lx86_64
DPADD+=	${LIBX86_64}
.elif ${MACHINE} == "i386"
LDADD+=	-li386
DPADD+=	${LIBI386}
.endif

SYMLINKS=	${X11BINDIR}/XFree86 ${X11BINDIR}/X

.include <bsd.x11.mk>

.if (${MACHINE_ARCH} == "arm" || ${MACHINE_ARCH} == "armeb") \
        && ${X11LOADABLE} != "no"
LDADD+= -larm
DPADD+= ${LIBARM}
.endif

LDADD+= -export-dynamic

.include <bsd.prog.mk>

XTODO=	# fix DPADD's
