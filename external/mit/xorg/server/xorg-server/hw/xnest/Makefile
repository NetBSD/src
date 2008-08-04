#	$NetBSD: Makefile,v 1.3 2008/08/04 04:32:17 mrg Exp $

.include <bsd.own.mk>

PROG=	Xnest

.include "../../Makefile.serverlib"
.include "../../Makefile.servermod"

.PATH:		${X11SRCDIR.xorg-server}/hw/xnest
.PATH:		${X11SRCDIR.xorg-server}/Xext
.PATH:		${X11SRCDIR.xorg-server}/Xi
.PATH:		${X11SRCDIR.xorg-server}/mi
.PATH:		${X11SRCDIR.xorg-server}/fb
SRCS=		Args.c Color.c Cursor.c Display.c Events.c Font.c \
		GC.c GCOps.c Handlers.c Init.c Keyboard.c \
		Pixmap.c Pointer.c Screen.c Visual.c Window.c \
		dpmsstubs.c stubs.c miinitext.c \
		fbcmap_mi.c

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/hw/xnest \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xorg-server}/mi \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${X11SRCDIR.xorg-server}/include \
		-I${XSERVERINCDIR} \
		-I${X11SRCDIR.xorg-server}/os \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${X11SRCDIR.xorg-server}/../include \
		-I${X11INCS.DIX}

CPPFLAGS+=	${X11FLAGS.OS_DEFINES} \
		${X11FLAGS.EXTENSION} \
		${X11FLAGS.DIX} \
		-DNO_HW_ONLY_EXTS -UXFree86LOADER -UMITSHM \
		-D__XKBDEFRULES__='"xorg"'

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/hw/xnest \
		-DHAVE_XNEST_CONFIG_H \
		-UGLXEXT

CPPFLAGS.Events.c=	-Wno-error
CPPFLAGS.Screen.c=	-Wno-error
CPPFLAGS.miinitext.c=	-UDPMSExtension

.include "../../Makefile.Xserver"

LDADD+= \
	${LDADD.XORG_CORE} \
	${LDADD.mi} \
	${LDADD.config} \
	-lXfont \
	-lXau \
	-lfontenc \
	-lpixman-1 \
	-lXdmcp \
	${LDADD.fb} \
	${LDADD.xfixes} \
	${LDADD.Xext} \
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
	${LDADD.dix/xpstubs} \
	${LDADD.os} \
	-lXext \
	-lX11 \
	-lXau \
	-lfreetype

DPADD+= \
	${DPADD.XORG_CORE} \
	${DPADD.mi} \
	${DPADD.config} \
	${LIBXFONT} \
	${LIBXAU} \
	${LIBFONTENC} \
	${LIBPIXMAN-1} \
	${LIBXDMCP} \
	${DPADD.fb} \
	${DPADD.xfixes} \
	${DPADD.Xext} \
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
	${DPADD.dix/xpstubs} \
	${DPADD.os} \
	${LIBXEXT} \
	${LIBX11} \
	${LIBXAU} \
	${LIBFREETYPE}

.if ${MACHINE_ARCH} == "i386"
LDADD+=	-li386
DPADD+=	${LIBI386}
.endif

.if ${MACHINE_ARCH} == "x86_64"
LDADD+=	-lx86_64
DPADD+=	${LIBX86_64}
.endif

# XXX
Xnest.1: Xnest.man.pre
	cp ${.ALLSRC} ${.TARGET}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
