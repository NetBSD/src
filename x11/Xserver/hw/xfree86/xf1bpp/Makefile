# $NetBSD: Makefile,v 1.3 2004/03/03 14:32:53 skrll Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules

.include "../../../Makefile.serverlib"

LIB=	xf1bpp
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/${LIB}
SRCS=	maskbits.c mfbbitblt.c mfbbltC.c mfbbltCI.c mfbbltG.c mfbbltO.c \
	mfbbltX.c mfbbres.c mfbbresd.c mfbbstore.c mfbclip.c mfbcmap.c \
	mfbfillarc.c mfbfillrct.c mfbfillsp.c mfbfont.c mfbgc.c mfbgetsp.c \
	mfbhrzvert.c mfbigbblak.c mfbigbwht.c mfbimage.c mfbline.c mfbmisc.c \
	mfbmodule.c mfbpablack.c mfbpainv.c mfbpawhite.c mfbpgbblak.c \
	mfbpgbinv.c mfbpgbwht.c mfbpixmap.c mfbplyblack.c mfbplyinv.c \
	mfbplywhite.c mfbpntwin.c mfbpolypnt.c mfbpushpxl.c mfbscrclse.c \
	mfbscrinit.c mfbseg.c mfbsetsp.c mfbteblack.c mfbtewhite.c mfbtileC.c \
	mfbtileG.c mfbwindow.c mfbzerarc.c

CPPFLAGS+=	-I${X11SRCDIR.xc} \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/programs/Xserver/mfb \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
                -I${X11SRCDIR.xc}/programs/Xserver/include \
                -I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/${LIB} \
                -I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
                -I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86 \
                -I${DESTDIR}${X11INCDIR}/X11 \
                -I${XSERVERINCDIR}

CPPFLAGS.mfbbltC.c=	-DMROP=Mcopy
CPPFLAGS.mfbbltCI.c=	-DMROP=McopyInverted
CPPFLAGS.mfbbltO.c=	-DMROP=Mor
CPPFLAGS.mfbbltG.c=	-DMROP=0
CPPFLAGS.mfbbltX.c=	-DMROP=Mxor
CPPFLAGS.mfbigbblak.c=	-DMFBIMAGEGLYPHBLT=mfbImageGlyphBltBlack -DOPEQ=\&=\~
CPPFLAGS.mfbigbwht.c=	-DMFBIMAGEGLYPHBLT=mfbImageGlyphBltWhite -DOPEQ=\|=
CPPFLAGS.mfbpablack.c=	-DMFBSOLIDFILLAREA=mfbSolidBlackArea \
			-DMFBSTIPPLEFILLAREA=mfbStippleBlackArea \
			-DOPEQ=\&=\~ -DEQWHOLEWORD=\=0
CPPFLAGS.mfbpainv.c=	-DMFBSOLIDFILLAREA=mfbSolidInvertArea \
			-DMFBSTIPPLEFILLAREA=mfbStippleInvertArea \
			-DOPEQ=\^= -DEQWHOLEWORD=\^=\~0
CPPFLAGS.mfbpawhite.c=	-DMFBSOLIDFILLAREA=mfbSolidWhiteArea \
			-DMFBSTIPPLEFILLAREA=mfbStippleWhiteArea -DOPEQ=\|= \
			-DEQWHOLEWORD=\=\~0
CPPFLAGS.mfbpgbblak.c=	-DMFBPOLYGLYPHBLT=mfbPolyGlyphBltBlack -DOPEQ=\&=\~
CPPFLAGS.mfbpgbinv.c=	-DMFBPOLYGLYPHBLT=mfbPolyGlyphBltInvert -DOPEQ=\^=
CPPFLAGS.mfbpgbwht.c=	-DMFBPOLYGLYPHBLT=mfbPolyGlyphBltWhite -DOPEQ=\|=
CPPFLAGS.mfbplyblack.c=	-DMFBFILLPOLY1RECT=mfbFillPolyBlack -DOPEQ=\&=\~ \
			-DEQWHOLEWORD=\=0
CPPFLAGS.mfbplyinv.c=	-DMFBFILLPOLY1RECT=mfbFillPolyInvert -DOPEQ=\^= \
			-DEQWHOLEWORD=\^=\~0
CPPFLAGS.mfbplywhite.c=	-DMFBFILLPOLY1RECT=mfbFillPolyWhite -DOPEQ=\|= \
			-DEQWHOLEWORD=\=\~0
CPPFLAGS.mfbseg.c=	-DPOLYSEGMENT
CPPFLAGS.mfbteblack.c=	-DMFBTEGLYPHBLT=mfbTEGlyphBltBlack -DOP=\~ \
			-DCLIPTETEXT=mfbImageGlyphBltBlack
CPPFLAGS.mfbtewhite.c=	-DMFBTEGLYPHBLT=mfbTEGlyphBltWhite -DOP= \
			-DCLIPTETEXT=mfbImageGlyphBltWhite
CPPFLAGS.mfbtileC.c=	-DMROP=Mcopy
CPPFLAGS.mfbtileG.c=	-DMROP=0

BUILDSYMLINKS=	mfbline.c mfbseg.c \
		mfbplygblt.c mfbpgbwht.c \
		mfbplygblt.c mfbpgbblak.c \
		mfbplygblt.c mfbpgbinv.c \
		mfbimggblt.c mfbigbwht.c \
		mfbimggblt.c mfbigbblak.c \
		mfbpntarea.c mfbpawhite.c \
		mfbpntarea.c mfbpablack.c \
		mfbpntarea.c mfbpainv.c \
		mfbtegblt.c mfbtewhite.c \
		mfbtegblt.c mfbteblack.c \
		mfbply1rct.c mfbplywhite.c \
		mfbply1rct.c mfbplyblack.c \
		mfbply1rct.c mfbplyinv.c \
		mfbblt.c mfbbltC.c \
		mfbblt.c mfbbltX.c \
		mfbblt.c mfbbltCI.c \
		mfbblt.c mfbbltO.c \
		mfbblt.c mfbbltG.c \
		mfbtile.c mfbtileC.c \
		mfbtile.c mfbtileG.c

GENSRCS=	maskbits.c mfbbitblt.c mfbblt.c mfbbres.c mfbbresd.c \
		mfbbstore.c mfbclip.c mfbcmap.c mfbfillarc.c mfbfillrct.c \
		mfbfillsp.c mfbfont.c mfbgc.c mfbgetsp.c mfbhrzvert.c \
		mfbimage.c mfbimggblt.c mfbline.c mfbmisc.c mfbpixmap.c \
		mfbply1rct.c mfbplygblt.c mfbpntarea.c mfbpntwin.c \
		mfbpolypnt.c mfbpushpxl.c mfbscrclse.c mfbscrinit.c \
		mfbsetsp.c mfbtegblt.c mfbtile.c mfbwindow.c mfbzerarc.c

.for f in ${GENSRCS}
CLEANFILES+=	${f}
${f}: ../../../mfb/${f}
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	@( \
		echo '#include "mfbmap.h"' ; \
		echo '#include "../../../mfb/${f}"' ; \
	) > ${.TARGET}
.endfor

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
