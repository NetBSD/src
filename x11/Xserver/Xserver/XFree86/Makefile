#	$NetBSD: Makefile,v 1.3 2003/12/23 03:33:41 rtr Exp $

.include <bsd.own.mk>

PROG=	XFree86
BINMODE=4711

MAN=	XFree86.1
# XF86Config.5


.PATH:  ${X11SRCDIR.xc}/programs/Xserver/hw/xfree86

.include "../Makefile.Xserver"

LDADD=	${LIBDIR.xkb}/xf86KillSrv.o ${LIBDIR.xkb}/xf86VT.o \
	${LIBDIR.xkb}/xf86Private.o ${LIBDIR.hw/xfree86/xf86}/xf86Init.o \
	${LIBDIR.hw/xfree86/xf86}/xf86IniExt.o

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
	-lz -lm

.if ${MACHINE} == "i386"
LDADD+=	-li386
.endif

.include <bsd.x11.mk>
.include <bsd.prog.mk>
