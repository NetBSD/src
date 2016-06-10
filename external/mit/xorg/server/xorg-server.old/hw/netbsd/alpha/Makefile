#	$NetBSD: Makefile,v 1.1.1.1 2016/06/10 03:42:14 mrg Exp $

NOMAN=	# defined (must come before bsd.own.mk)

.include <bsd.own.mk>

PROG=	XalphaNetBSD
BINMODE=4711

SYMLINKS=	${X11BINDIR}/X68k ${X11BINDIR}/X

.include "../../../Makefile.serverlib"
.include "../../../Makefile.servermod"

.PATH:	${X11SRCDIR.xorg-server}/hw/netbsd/alpha
SRCS=	alphaFbs.c alphaIo.c alphaCursor.c alphaKbd.c alphaMouse.c \
	alphaKeyMap.c alphaInit.c

.PATH:	${X11SRCDIR.xorg-server}/hw/netbsd/alpha/sfb
SRCS+=	alphaSFB.c sfbgc.c sfbbitblt.c sfbblt.c sfbwindow.c sfbsimpleblt.c \
	sfb32gc.c sfb32bitblt.c sfb32blt.c sfb32window.c

CPPFLAGS.sfb32gc.c=	-DPSZ=32
CPPFLAGS.sfb32bitblt.c=	-DPSZ=32
CPPFLAGS.sfb32blt.c=	-DPSZ=32
CPPFLAGS.sfb32window.c=	-DPSZ=32

.PATH:	${X11SRCDIR.xorg-server}/hw/netbsd/alpha/tga
SRCS+=	alphaTGA.c tgagc.c tgabitblt.c tgablt.c tgawindow.c tgasimpleblt.c \
	tga32gc.c tga32bitblt.c tga32blt.c tga32window.c

CPPFLAGS.tga32gc.c=	-DPSZ=32
CPPFLAGS.tga32bitblt.c=	-DPSZ=32
CPPFLAGS.tga32blt.c=	-DPSZ=32
CPPFLAGS.tga32window.c=	-DPSZ=32

.PATH:	${X11SRCDIR.xorg-server}/Xext
SRCS+=	dpmsstubs.c

.PATH:	${X11SRCDIR.xorg-server}/Xi
SRCS+=	stubs.c

.PATH:	${X11SRCDIR.xorg-server}/mi
SRCS+=	miinitext.c

.PATH:	${X11SRCDIR.xorg-server}/fb
SRCS+=	fbcmap_mi.c

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/include \
		-I${X11SRCDIR.xorg-server}/hw/netbsd/alpha \
		-I${X11SRCDIR.xorg-server}/hw/netbsd/alpha/sfb \
		-I${X11SRCDIR.xorg-server}/hw/netbsd/alpha/tga \
		-I${X11SRCDIR.xorg-server}/fb \
		-I${X11SRCDIR.xorg-server}/mi \
		-I${X11SRCDIR.xorg-server}/render \
		-I${DESTDIR}${X11INCDIR}/X11 \

CPPFLAGS+=	-UXFree86LOADER -UXF86VIDMODE -UXFreeXDGA -UXF86MISC \
		-UXF86DRI -UXF86BIGFONT

#CPPFLAGS.alphaKbd.c=	-Wno-error

BUILDSYMLINKS+=	\
		sfbgc.c sfb32gc.c \
		sfbbitblt.c sfb32bitblt.c \
		sfbblt.c sfb32blt.c \
		sfbwindow.c sfb32window.c \
		tgagc.c tga32gc.c \
		tgabitblt.c tga32bitblt.c \
		tgablt.c tga32blt.c \
		tgawindow.c tga32window.c

.include "../../../Makefile.Xserver"

LDADD+= ${LDADD.dix} \
	${LDADD.fb} \
	${LDADD.mi} \
	${LDADD.xkb} \
	${LDADD.xkbstubs} \
	${LDADD.randr} \
	${LDADD.Xext} \
	${LDADD.composite} \
	${LDADD.render} \
	${LDADD.os} \
	${LDADD.xfixes} \
	${LDADD.damageext} \
	${LDADD.miext/damage} \
	${LDADD.miext/shadow} \
	${LDADD.miext/sync} \
	${LDADD.Xi} \
	-lXfont \
	-lpixman-1 \
	-lXext \
	-lX11 \
	-lz \
	-lm

DPADD+= ${LDADD.dix} \
	${DPADD.fb} \
	${DPADD.mi} \
	${DPADD.xkb} \
	${DPADD.randr} \
	${DPADD.Xext} \
	${DPADD.composite} \
	${DPADD.render} \
	${DPADD.os} \
	${DPADD.xfixes} \
	${DPADD.damageext} \
	${DPADD.miext/damage} \
	${DPADD.miext/shadow} \
	${DPADD.miext/sync} \
	${DPADD.Xi} \
	${LIBXFONT} \
	${LIBPIXMAN-1} \
	${LIBXEXT} \
	${LIBX11} \
	${LIBZ} \
	${LIBM}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
