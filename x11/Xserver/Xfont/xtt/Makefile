#	$NetBSD: Makefile,v 1.4.4.2 2004/11/21 10:01:26 rtr Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/fonts

.include "../../Makefile.serverlib"
.include "../../Makefile.servermod"

LIB=	xtt
.PATH:	${X11SRCDIR.xc}/extras/X-TrueType
.PATH:	${X11SRCDIR.xc}/extras/FreeType/lib/arch/unix
.PATH:	${X11SRCDIR.xc}/extras/FreeType/lib/extend
.PATH:	${X11SRCDIR.xc}/lib/font/X-TrueType/module

# XXX we are building module only version
SRCS=	xttprop.c xttfuncs.c xttcconv.c xttcap.c xttmisc.c \
	xttmodule.c

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/fonts/codeconv \
		-I${X11SRCDIR.xc}/lib/font/X-TrueType \
		-I${X11SRCDIR.xc}/extras/FreeType/lib \
		-I${X11SRCDIR.xc}/extras/FreeType/lib/extend \
		-I${X11SRCDIR.xc}/lib/font/FreeType \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/lib/font/include \
		-I${X11SRCDIR.xc}/extras/X-TrueType \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/lib/font/fontcache \
		-I${XSERVERINCDIR}

CPPFLAGS+=	-DTT_MAKE_OPTION_SINGLE_OBJECT=1 \
		-DTT_CONFIG_OPTION_EXTEND_ENGINE \
		-DOPT_ENCODINGS \
		-DXORG_VERSION_CURRENT=${XORG_VERSION_CURRENT}

CPPFLAGS+=	-DFONTMODULE \
		-DCCONV_MODULE \
		-DCCONV_MODULE_DIR=\"/usr/X11R6/lib/modules/codeconv\"

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>

SUBDIR=	ARABIC BIG5HKSCS GBK ISO8859_10 ISO8859_15 ISO8859_4 ISO8859_7 \
	JISX0201 KOI8 MULEENCODING VISCII ARMSCII8 DOSENCODING GEORGIAN \
	ISO8859_11 ISO8859_2 ISO8859_5 ISO8859_8 JISX0208 KSC5601 BIG5 \
	GB2312 ISO8859_1 ISO8859_14 ISO8859_3 ISO8859_6 ISO8859_9 \
	JISX0212 KSCJOHAB TCVN

.include <bsd.subdir.mk>
