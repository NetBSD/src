#	$NetBSD: Makefile,v 1.2 2003/09/23 23:21:07 lukem Exp $

.include <bsd.own.mk>

PROG=	XFree86
.PATH:  ${X11SRCDIR.xc}/programs/Xserver/hw/xfree86
MAN=	XFree86.1 XF86Config.5

.include "../Makefile.Xserver"

LDADD+=	${LIBDIR.xkb}/xf86KillSrv.o ${LIBDIR.xkb}/xf86VT.o \
	${LIBDIR.xkb}/xf86Private.o \
#	../../programs/Xserver/hw/xfree86/common/xf86Init.o
#	../../programs/Xserver/hw/xfree86/common/xf86IniExt.o
#	../../programs/Xserver/hw/xfree86/common/libxf86.a
#	../../programs/Xserver/hw/xfree86/parser/libxf86config.a
#	../../programs/Xserver/hw/xfree86/os-support/libxf86_os.a
#	../../programs/Xserver/hw/xfree86/loader/libloader.a
#	../../programs/Xserver/hw/xfree86/common/libxf86.a

LDADD+=	${LDADD.dix} ${LDADD.os} -lXau -lXdmcp \
#	../../lib/font/fontbase.o
#	../../lib/font/libfontbase.a

LDADD+=	${LDADD.Xext/exts} ${LDADD.xkb} ${LDADD.xinput} ${LDADD.lbx} -llbxutil \
#	../../programs/Xserver/hw/xfree86/common/libxf86.a

LDADD+=	${LDADD.Xext/exts} ${LDADD.xkb} ${LDADD.xinput} ${LDADD.lbx} -llbxutil \
	${LDADD.randr} ${LDADD.render} ${LDADD.dix/xpstubs} ${LDADD.mi} \
	${LDADD.Xext/exts} ${LDADD.xkb} ${LDADD.xinput} ${LDADD.lbx} -llbxutil \
	${LDADD.randr} ${LDADD.render} \
#	../../programs/Xserver/hw/xfree86/os-support/libxf86_os.a

LDADD+=	-lz -lm -li386

.include <bsd.x11.mk>
.include <bsd.prog.mk>
