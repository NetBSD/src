#	$NetBSD: Makefile,v 1.13.2.3 2004/12/07 11:00:33 rtr Exp $

.include <bsd.own.mk>

SUBDIR=	ddc doc dummy etc fbdevhw i2c int10 loader pcidata rac \
	ramdac scanpci shadowfb vbe vgahw xaa xf1bpp xf24_32bpp \
	xf4bpp xf86 xf86_os xf86config xf8_16bpp xf8_32bpp xloader

.if ${MKX11LOADABLE} != "no"
SUBDIR+=drivers input
.else
SUBDIR+=driver idriver
.endif

# build drm if building dri
.if ${X11DRI} != "no"
SUBDIR+=	drm
.endif

SUBDIR+=.WAIT bin

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86

BUILDSYMLINKS=	xorgconf.cpp xorg.cfg.cpp \
		XF98Conf.cpp XF98Config.cpp

CPPSCRIPTS=	xorg.cfg XF98Config

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

CPPSCRIPTFLAGS_XF86Config=${CPPSCRIPTFLAGS} \
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
