#	$NetBSD: Makefile,v 1.3 2003/09/23 23:21:17 lukem Exp $

.include "../Makefile.serverlib"

.include <bsd.own.mk>

LIB=		xkb
.PATH:		${X11SRCDIR.xc}/programs/Xserver/xkb
.PATH:		${X11SRCDIR.xc}/lib/X11
.PATH:		${X11SRCDIR.xc}/lib/xkbfile

SRCS=		XKBAlloc.c XKBGAlloc.c XKBMAlloc.c XKBMisc.c ddxBeep.c \
		ddxConfig.c ddxCtrls.c ddxDevBtn.c ddxFakeBtn.c ddxFakeMtn.c \
		ddxInit.c ddxKeyClick.c ddxKillSrv.c ddxLEDs.c ddxList.c \
		ddxLoad.c ddxPrivate.c ddxVT.c maprules.c xkb.c xkbAccessX.c \
		xkbActions.c xkbEvents.c xkbInit.c xkbLEDs.c xkbPrKeyEv.c \
		xkbPrOtherEv.c xkbSwap.c xkbUtils.c xkbconfig.c xkberrs.c \
		xkbfmisc.c xkbout.c xkbtext.c xkmread.c

BUILDSYMLINKS+=	xkbmisc.c xkbfmisc.c

XSERVERINCDIR!=	cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${XSERVERINCDIR} \
		-I${DESTDIR}/${X11INCDIR}/X11 \
		-I${DESTDIR}/${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc}/programs/Xserver/Xext \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common

CPPFLAGS+=	-DXKB_IN_SERVER

CPPFLAGS.xkbInit.c=	-DXKB_BASE_DIRECTORY=\"${X11LIBDIR}/xkb\" \
			-DXKB_DFLT_DISABLED=0

COPTS.XKBAlloc.c=	-Wno-error	# XXX gcc 3.3
COPTS.XKBMAlloc.c=	-Wno-error	# XXX gcc 3.3
COPTS.xkb.c=		-Wno-error	# XXX gcc 3.3

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
