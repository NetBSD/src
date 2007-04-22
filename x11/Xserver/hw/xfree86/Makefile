#	$NetBSD: Makefile,v 1.15 2007/04/22 23:42:38 macallan Exp $

.include <bsd.own.mk>

SUBDIR=	ddc doc drivers dummy etc fbdevhw i2c input int10 loader \
	pcidata rac ramdac scanpci shadowfb vbe vgahw xaa xf1bpp \
	xf24_32bpp xf4bpp xf86 xf86_os xf86config xf8_16bpp \
	xf8_32bpp

.if ${MACHINE} == "sparc64"
SUBDIR+=	xf8_32wid
.endif

.if ${MACHINE} == "sparc"
SUBDIR+=        xf8_32wid
.endif

SUBDIR+=	xloader .WAIT bin

.if ${X11DRI} != "no"
SUBDIR+=	drm
.endif

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86

BUILDSYMLINKS=	XF86Conf.cpp XF86Config.eg.cpp \
		XF98Conf.cpp XF86Config.98.cpp

CPPSCRIPTS=	XF86Config.eg XF86Config.98

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

CPPSCRIPTFLAGS_XF86Config.98=${CPPSCRIPTFLAGS} \
		-DFREEBSDMOUSEDEV='\#    Option	"Device"	"/dev/mse0"' \
		-DNETBSDNEWMOUSEDEV='Option	"Device"	"/dev/lms0"' \
		-DNETBSDOLDMOUSEDEV='\#    Option	"Device"	"/dev/mms0"' \
		-DLINUXMOUSEDEV='\#    Option	"Device"	"/dev/mouse"' \

${CPPSCRIPTS}: Makefile

FILES=		${CPPSCRIPTS}
FILESDIR=	${X11LIBDIR}

.include <bsd.x11.mk>
.include <bsd.files.mk>
.include <bsd.subdir.mk>
.include <bsd.obj.mk>
