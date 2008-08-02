#	$NetBSD: Makefile,v 1.1 2008/08/02 20:41:29 mrg Exp $

.include "../Makefile.serverlib"
.include "../Makefile.servermod"

LIB=		xkb

.PATH:		${X11SRCDIR.xorg-server}/${LIB}
.PATH:		${X11SRCDIR.X11}/src
.PATH:		${X11SRCDIR.xkbfile}/src
DDXSRCS=	ddxBeep.c ddxCtrls.c ddxFakeBtn.c ddxFakeMtn.c \
		ddxInit.c ddxKeyClick.c ddxLEDs.c ddxLoad.c \
		ddxList.c ddxDevBtn.c
DIXSRCS=	xkb.c xkbUtils.c xkbEvents.c xkbAccessX.c xkbSwap.c \
		xkbLEDs.c xkbInit.c xkbActions.c xkbPrKeyEv.c
XKBFILESRCS=	maprules.c xkmread.c xkbtext.c xkbfmisc.c xkberrs.c \
		xkbout.c
X11SRCS=	XKBMisc.c XKBAlloc.c XKBGAlloc.c XKBMAlloc.c

SRCS=		${DDXSRCS} ${DIXSRCS} ${XKBFILESRCS} ${X11SRCS}

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/include \
		-I${XSERVERINCDIR} \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${X11SRCDIR.xorg-server}/Xext/extmod \
		-I${X11SRCDIR.xorg-server}/mi \
		-I${X11SRCDIR.xorg-server}/${LIB} \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/common \
		-I${X11SRCDIR.xorg-server}/../include

CPPFLAGS+=	-DXKB_IN_SERVER -DHAVE_XKB_CONFIG_H

CPPFLAGS.xkbInit.c=	-DXKB_BASE_DIRECTORY=\"${X11LIBDIR}/xkb\" \
			-DXKB_BIN_DIRECTORY=\"${X11BINDIR}\" \
			-DXKM_OUTPUT_DIR=\"${X11LIBDIR}/xkb/compiled\" \
			-DXKB_DFLT_DISABLED=0

COPTS.XKBAlloc.c=	-Wno-error	# XXX gcc 3.3
COPTS.XKBMAlloc.c=	-Wno-error	# XXX gcc 3.3
COPTS.xkb.c=		-Wno-error	# XXX gcc 3.3
COPTS.xkbUtils.c=	-Wno-error	# XXX gcc 3.3

# XXX?
.for _S _T in	ddxKillSrv.c xf86KillSrv.c \
		ddxVT.c xf86VT.c \
		ddxPrivate.c xf86Private.c
BUILDSYMLINKS+=		${_S} ${_T}
BUILDOBJS+=		${_T:R}.o
CPPFLAGS.${_T}=		-DXF86DDXACTIONS
.endfor

CLEANFILES+=		${BUILDOBJS}
realall: ${BUILDOBJS}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
