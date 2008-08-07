#	$NetBSD: Makefile,v 1.11 2008/08/07 06:37:20 mrg Exp $

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

CPPFLAGS+=	${X11FLAGS.OS_DEFINES} ${X11FLAGS.DIX} \
		${X11FLAGS.EXTENSION}

LDADD+=	${LDADD.dix} \
	${LDADD.fb} \
	${LDADD.mi} \
	${LDADD.xfixes} \
	${LDADD.Xext} \
	${LDADD.config} \
	${LDADD.dbe} \
	${LDADD.XTrap} \
	${LDADD.record} \
	${LDADD.render} \
	${LDADD.randr} \
	${LDADD.damageext} \
	${LDADD.miext/damage} \
	${LDADD.miext/shadow} \
	${LDADD.Xi} \
	${LDADD.xkb} \
	${LDADD.composite} \
	${LDADD.os} \
	${LDADD.dix/xpstubs} \
	${LDADD.hw/xfree86/fb} \
	${LDADD.hw/xfree86/init} \
	${LDADD.hw/xfree86/loader} \
	${LDADD.hw/xfree86/rac} \
	${LDADD.hw/xfree86/xf86config} \
	${LDADD.hw/xfree86/dixmods} \
	${LDADD.hw/xfree86/xf86modes} \
	${LDADD.hw/xfree86/ramdac} \
	${LDADD.hw/xfree86/ddc} \
	${LDADD.hw/xfree86/i2c} \
	-lXfont \
	-lfreetype \
	-lXau \
	-lfontenc \
	${LIBCWRAPPER} \
	-lpixman-1 \
	-lXdmcp \
	-lz \
	-lm

DPADD+=	${DPADD.dix} \
	${DPADD.fb} \
	${DPADD.mi} \
	${DPADD.xfixes} \
	${DPADD.Xext} \
	${DPADD.config} \
	${DPADD.dbe} \
	${DPADD.XTrap} \
	${DPADD.record} \
	${DPADD.render} \
	${DPADD.randr} \
	${DPADD.damageext} \
	${DPADD.miext/damage} \
	${DPADD.miext/shadow} \
	${DPADD.Xi} \
	${DPADD.xkb} \
	${DPADD.composite} \
	${DPADD.os} \
	${DPADD.dix/xpstubs} \
	${DPADD.hw/xfree86/fb} \
	${DPADD.hw/xfree86/init} \
	${DPADD.hw/xfree86/loader} \
	${DPADD.hw/xfree86/rac} \
	${DPADD.hw/xfree86/xf86config} \
	${DPADD.hw/xfree86/dixmods} \
	${DPADD.hw/xfree86/xf86modes} \
	${DPADD.hw/xfree86/ramdac} \
	${DPADD.hw/xfree86/ddc} \
	${DPADD.hw/xfree86/i2c} \
	${LIBCWRAPPER} \
	${LIBXFONT} ${LIBFREETYPE} ${LIBXAU} ${LIBFONTENC} \
	${LIBPIXMAN1} ${LIBXDMCP} ${LIBZ} ${LIBM}

# XXX
#Xorg.1: Xorg.man.pre
#	cp ${.ALLSRC} ${.TARGET}
MKMAN=	no

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
