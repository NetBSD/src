#	$NetBSD: Makefile,v 1.3 2003/09/23 23:21:13 lukem Exp $

XMODULEDIR=	${X11ROOTDIR}/lib/modules

.include "../Makefile.serverlib"

.include <bsd.own.mk>

LIB=		mfb
.PATH:		${X11SRCDIR.xc}/programs/Xserver/mfb

SRCS=		maskbits.c mfbbitblt.c mfbbltC.c mfbbltCI.c mfbbltG.c \
		mfbbltO.c mfbbltX.c mfbbres.c mfbbresd.c mfbbstore.c \
		mfbclip.c mfbcmap.c mfbfillarc.c mfbfillrct.c \
		mfbfillsp.c mfbfont.c mfbgc.c mfbgetsp.c mfbhrzvert.c \
		mfbigbblak.c mfbigbwht.c mfbimage.c mfbline.c \
		mfbmisc.c mfbmodule.c mfbpablack.c mfbpainv.c \
		mfbpawhite.c mfbpgbblak.c mfbpgbinv.c mfbpgbwht.c \
		mfbpixmap.c mfbplyblack.c mfbplyinv.c mfbplywhite.c \
		mfbpntwin.c mfbpolypnt.c mfbpushpxl.c mfbscrclse.c \
		mfbscrinit.c mfbseg.c mfbsetsp.c mfbteblack.c \
		mfbtewhite.c mfbtileC.c mfbtileG.c mfbwindow.c \
		mfbzerarc.c

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

XSERVERINCDIR!=	cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/mfb \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${XSERVERINCDIR} \
		-I${DESTDIR}/${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/include/fonts

CPPFLAGS.mfbseg.c=	-DPOLYSEGMENT 
CPPFLAGS.mfbpgbwht.c=	-DMFBPOLYGLYPHBLT=mfbPolyGlyphBltWhite -DOPEQ=MFB_OPEQ_WHITE 
CPPFLAGS.mfbpgbblak.c=	-DMFBPOLYGLYPHBLT=mfbPolyGlyphBltBlack -DOPEQ=MFB_OPEQ_BLACK 
CPPFLAGS.mfbpgbinv.c=	-DMFBPOLYGLYPHBLT=mfbPolyGlyphBltInvert -DOPEQ=MFB_OPEQ_INVERT 
CPPFLAGS.mfbigbwht.c=	-DMFBIMAGEGLYPHBLT=mfbImageGlyphBltWhite -DOPEQ=MFB_OPEQ_WHITE 
CPPFLAGS.mfbigbblak.c=	-DMFBIMAGEGLYPHBLT=mfbImageGlyphBltBlack -DOPEQ=MFB_OPEQ_BLACK 
CPPFLAGS.mfbpawhite.c=	-DMFBSOLIDFILLAREA=mfbSolidWhiteArea -DMFBSTIPPLEFILLAREA=mfbStippleWhiteArea -DOPEQ=MFB_OPEQ_WHITE -DEQWHOLEWORD=MFB_EQWHOLEWORD_WHITE 
CPPFLAGS.mfbpablack.c=	-DMFBSOLIDFILLAREA=mfbSolidBlackArea -DMFBSTIPPLEFILLAREA=mfbStippleBlackArea -DOPEQ=MFB_OPEQ_BLACK -DEQWHOLEWORD=MFB_EQWHOLEWORD_BLACK 
CPPFLAGS.mfbpainv.c=	-DMFBSOLIDFILLAREA=mfbSolidInvertArea -DMFBSTIPPLEFILLAREA=mfbStippleInvertArea -DOPEQ=MFB_OPEQ_INVERT -DEQWHOLEWORD=MFB_EQWHOLEWORD_INVERT 
CPPFLAGS.mfbtewhite.c=	-DMFBTEGLYPHBLT=mfbTEGlyphBltWhite -DOP=MFB_OP_WHITE -DCLIPTETEXT=mfbImageGlyphBltWhite 
CPPFLAGS.mfbteblack.c=	-DMFBTEGLYPHBLT=mfbTEGlyphBltBlack -DOP=MFB_OP_BLACK -DCLIPTETEXT=mfbImageGlyphBltBlack 
CPPFLAGS.mfbplywhite.c=	-DMFBFILLPOLY1RECT=mfbFillPolyWhite -DOPEQ=MFB_OPEQ_WHITE -DEQWHOLEWORD=MFB_EQWHOLEWORD_WHITE 
CPPFLAGS.mfbplyblack.c=	-DMFBFILLPOLY1RECT=mfbFillPolyBlack -DOPEQ=MFB_OPEQ_BLACK -DEQWHOLEWORD=MFB_EQWHOLEWORD_BLACK 
CPPFLAGS.mfbplyinv.c=	-DMFBFILLPOLY1RECT=mfbFillPolyInvert -DOPEQ=MFB_OPEQ_INVERT -DEQWHOLEWORD=MFB_EQWHOLEWORD_INVERT 
CPPFLAGS.mfbbltC.c=	-DMROP=Mcopy 
CPPFLAGS.mfbbltX.c=	-DMROP=Mxor 
CPPFLAGS.mfbbltCI.c=	-DMROP=McopyInverted 
CPPFLAGS.mfbbltO.c=	-DMROP=Mor 
CPPFLAGS.mfbbltG.c=	-DMROP=0 
CPPFLAGS.mfbtileC.c=	-DMROP=Mcopy 
CPPFLAGS.mfbtileG.c=	-DMROP=0

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
