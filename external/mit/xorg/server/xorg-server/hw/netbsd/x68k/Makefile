#	$NetBSD: Makefile,v 1.9 2020/08/05 15:48:09 tsutsui Exp $

.include <bsd.own.mk>

PROG=		X68k
BINMODE=	4711

SYMLINKS=	${X11BINDIR}/X68k ${X11BINDIR}/X

FILES=		X68kConfig
FILESDIR=	${X11ETCDIR}

.include "../../../Makefile.serverlib"
.include "../../../Makefile.servermod"

WARNS?=	2

.PATH:	${X11SRCDIR.xorg-server}/hw/netbsd/x68k
SRCS=	x68kInit.c x68kConfig.c x68kIo.c x68kMouse.c x68kKbd.c x68kKeyMap.c \
	x68kFb.c x68kGraph.c x68kText.c

.PATH:	${X11SRCDIR.xorg-server}/Xi
SRCS+=	stubs.c

.PATH:	${X11SRCDIR.xorg-server}/mi
SRCS+=	miinitext.c

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/hw/netbsd/x68k \
		${X11INCS.DIX}

CPPFLAGS+=	-UXFree86LOADER -UXF86VIDMODE -UXFreeXDGA -UXF86MISC \
		-UXF86DRI -UXF86BIGFONT

CPPFLAGS+=	-UHAVE_XORG_CONFIG_H
CPPFLAGS+=	-DHAVE_STRCASECMP

CPPFLAGS.x68kConfig.c=	 -DX11_LIBDIR=\"${X11ETCDIR}\"

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
	${LDADD.miext/sync} \
	${LDADD.Xi} \
	-lXfont2 \
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
	${DPADD.miext/sync} \
	${DPADD.Xi} \
	${LIBXFONT2} \
	${LIBPIXMAN-1} \
	${LIBXEXT} \
	${LIBX11} \
	${LIBZ} \
	${LIBM}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
