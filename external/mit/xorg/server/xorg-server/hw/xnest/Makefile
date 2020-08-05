#	$NetBSD: Makefile,v 1.23 2020/08/05 15:48:09 tsutsui Exp $

.include <bsd.own.mk>

PROG=	Xnest

.include "../../Makefile.serverlib"
.include "../../Makefile.servermod"

.PATH:		${X11SRCDIR.xorg-server}/hw/xnest
.PATH:		${X11SRCDIR.xorg-server}/hw/xnest/man
.PATH:		${X11SRCDIR.xorg-server}/Xext
.PATH:		${X11SRCDIR.xorg-server}/Xi
.PATH:		${X11SRCDIR.xorg-server}/mi
.PATH:		${X11SRCDIR.xorg-server}/fb
SRCS=		Args.c Color.c Cursor.c Display.c Events.c Font.c \
		GC.c GCOps.c Handlers.c Init.c Keyboard.c \
		Pixmap.c Pointer.c Screen.c Visual.c Window.c \
		stubs.c miinitext.c

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/hw/xnest \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xorg-server}/mi \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${X11SRCDIR.xorg-server}/include \
		-I${X11SRCDIR.xorg-server}/os \
		-I${X11SRCDIR.xorg-server}/present \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/common \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${X11SRCDIR.xorg-server}/../include \
		${X11INCS.DIX}

CPPFLAGS+=	${X11FLAGS.OS_DEFINES} \
		${X11FLAGS.EXTENSION} \
		${X11FLAGS.DIX} \
		-UXFree86LOADER -UMITSHM \
		-D__XKBDEFRULES__='"xorg"'

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/hw/xnest \
		-DHAVE_XNEST_CONFIG_H \
		-UGLXEXT

CPPFLAGS.Events.c=	-Wno-error
CPPFLAGS.Screen.c=	-Wno-error
CPPFLAGS.Color.c=	-Wno-error
CPPFLAGS.Window.c=	-Wno-error
CPPFLAGS.miinitext.c=	-UDPMSExtension

.include "../../Makefile.Xserver"

LDADD+=	${LDADD.dix} \
	${LDADD.fb} \
	${LDADD.mi} \
	${LDADD.config} \
	${LDADD.xfixes} \
	${LDADD.Xext} \
	${LDADD.dbe} \
	${LDADD.record} \
	${LDADD.randr} \
	${LDADD.damageext} \
	${LDADD.miext/damage} \
	${LDADD.miext/shadow} \
	${LDADD.miext/sync} \
	${LDADD.render} \
	${LDADD.Xi} \
	${LDADD.xkb} \
	${LDADD.xkbstubs} \
	${LDADD.composite} \
	${LDADD.os} \
	${LDADD.glx} \
	${LDADD.present} \
	-lXfont2 -lXext -lfontenc \
	-lpixman-1 -lX11 -lXau \
	-lXdmcp -lfreetype -lm

DPADD+=	${LDADD.dix} \
	${DPADD.fb} \
	${DPADD.mi} \
	${DPADD.config} \
	${DPADD.xfixes} \
	${DPADD.Xext} \
	${DPADD.dbe} \
	${DPADD.record} \
	${DPADD.render} \
	${DPADD.randr} \
	${DPADD.damageext} \
	${DPADD.miext/damage} \
	${DPADD.miext/shadow} \
	${DPADD.miext/sync} \
	${DPADD.Xi} \
	${DPADD.xkb} \
	${DPADD.composite} \
	${DPADD.os} \
	${DPADD.glx} \
	${DPADD.present} \
	${LIBXFONT} ${LIBXEXT} ${LIBFONTENC} \
	${LIBPIXMAN-1} ${LIBX11} ${LIBXAU} \
	${LIBXDMCP} ${LIBFREETYPE} ${LIBM}

LDADD+=	-lcrypto
DPADD+=	${LIBCRYPTO}

.if ${MACHINE_ARCH} == "i386"
LDADD+=	-li386
DPADD+=	${LIBI386}
.endif

.if ${MACHINE_ARCH} == "x86_64"
LDADD+=	-lx86_64
DPADD+=	${LIBX86_64}
.endif

.if ${MACHINE_ARCH} == "alpha"
LDADD+=	-lalpha
DPADD+=	${LIBALPHA}
.endif

COPTS.miinitext.c+=	-Wno-error	# XXX xf86bigfstr.h

.include <bsd.x11.mk>
.include <bsd.prog.mk>
