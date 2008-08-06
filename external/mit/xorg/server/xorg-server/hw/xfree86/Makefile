#	$NetBSD: Makefile,v 1.10 2008/08/06 07:38:15 mrg Exp $

SUBDIR=		common ddc dixmods doc dri dummy i2c int10 loader modes \
		os-support rac ramdac scanpci shadowfb vbe vgahw \
		x86emu xaa xf1bpp xf4bpp xf8_16bpp xf8_32bpp

SUBDIR+=	.WAIT utils

.include <bsd.own.mk>

.include "../../Makefile.Xserver"

PROG=		Xorg

.PATH:		${X11SRCDIR.xorg-server}/hw/xfree86
SRCS=		xorg.c

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xorg-server}/fb \
		-I${X11SRCDIR.xorg-server}/mfb \
		-I${X11SRCDIR.xorg-server}/mi \
		-I${X11SRCDIR.xorg-server}/include \
		-I${XSERVERINCDIR} \
		-I${X11SRCDIR.xorg-server}/os \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${DESTDIR}${X11INCDIR}/xorg \
		-I${X11SRCDIR.xorg-server}/render \
		-DHAVE_DIX_CONFIG_H

CPPFLAGS+=	${X11FLAGS.OS_DEFINES} ${X11FLAGS.DIX}

LDADD+=	${LDADD.hw/xfree86/dix} \
	${LDADD.hw/xfree86/fb} \
	${LDADD.hw/xfree86/init} \
	${LDADD.hw/xfree86/loader} \
	${LDADD.hw/xfree86/rac} \
	${LDADD.hw/xfree86/xf86config} \
	${LDADD.hw/xfree86/dixmods} \
	${LDADD.hw/xfree86/xf86modes} \
	${LDADD.hw/xfree86/ramdac} \
	${LDADD.hw/xfree86/ddc} \
	${LDADD.hw/xfree86/i2c}

DPADD+=	${DPADD.hw/xfree86/dix} \
	${DPADD.hw/xfree86/fb} \
	${DPADD.hw/xfree86/init} \
	${DPADD.hw/xfree86/loader} \
	${DPADD.hw/xfree86/rac} \
	${DPADD.hw/xfree86/xf86config} \
	${DPADD.hw/xfree86/dixmods} \
	${DPADD.hw/xfree86/xf86modes} \
	${DPADD.hw/xfree86/ramdac} \
	${DPADD.hw/xfree86/ddc} \
	${DPADD.hw/xfree86/i2c}

# XXX
Xorg.1: Xorg.man.pre
	cp ${.ALLSRC} ${.TARGET}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
