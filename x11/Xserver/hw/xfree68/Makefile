#	$NetBSD: Makefile,v 1.1 2004/03/28 15:31:32 jdc Exp $

.include <bsd.own.mk>

SUBDIR=	FBDev

CPPSCRIPTFLAGS=	-DRGBPATH=\"${X11LIBDIR}/rgb\" \
		-DMISCFONTPATH=\"${X11FONTDIR}/misc/\" \
		-DLOCALFONTPATH=\"${X11FONTDIR}/local/\" \
		-DT1FONTPATH=\"${X11FONTDIR}/Type1/\" \
		-DCIDFONTPATH=\"${X11FONTDIR}/CID/\" \
		-DSPFONTPATH=\"${X11FONTDIR}/Speedo/\" \
		-DDPI75FONTPATH=\"${X11FONTDIR}/75dpi/\" \
		-DDPI100FONTPATH=\"${X11FONTDIR}/100dpi/\" \
		-DDPI75USFONTPATH=\"${X11FONTDIR}/75dpi/:unscaled\" \
		-DDPI100USFONTPATH=\"${X11FONTDIR}/100dpi/:unscaled\" \
		-DMANPAGE=${X11MANDIR}/cat5/XF86Config.0 \
		-DMODULEPATH=\"${X11USRLIBDIR}/modules\"

${CPPSCRIPTS}: Makefile

FILES=		${CPPSCRIPTS}
FILESDIR=	${X11LIBDIR}

.include <bsd.x11.mk>
.include <bsd.files.mk>
.include <bsd.subdir.mk>
