#	$NetBSD: Makefile,v 1.1.1.1 2003/09/12 02:52:27 lukem Exp $

.include <bsd.own.mk>

LIB=	Xmu
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	AllCmap.c Atoms.c ClientWin.c Clip.c CloseHook.c CmapAlloc.c \
	CrCmap.c CrPixFBit.c CursorName.c CvtCache.c CvtStdSel.c \
	DefErrMsg.c DelCmap.c DisplayQue.c Distinct.c DrRndRect.c \
	DrawLogo.c EditresCom.c ExtAgent.c FToCback.c GetHost.c \
	GrayPixmap.c Initer.c LocBitmap.c Lookup.c LookupCmap.c \
	Lower.c RdBitF.c ScrOfWin.c ShapeWidg.c StdCmap.c StrToBS.c \
	StrToBmap.c StrToCurs.c StrToGrav.c StrToJust.c StrToLong.c \
	StrToOrnt.c StrToShap.c StrToWidg.c UpdMapHint.c VisCmap.c \
	WidgetNode.c Xct.c

INCS=	Atoms.h CharSet.h CloseHook.h Converters.h CurUtil.h \
	CvtCache.h DisplayQue.h Drawing.h Editres.h EditresP.h \
	Error.h ExtAgent.h Initer.h Lookup.h Misc.h StdCmap.h \
	StdSel.h SysUtil.h WidgetNode.h WinUtil.h Xct.h Xmu.h
INCSDIR=${X11INCDIR}/X11/Xmu

CPPFLAGS+=	${X11FLAGS.THREADLIB}
CPPFLAGS+=	-DHAS_SNPRINTF
CPPFLAGS+=	-DXTSTRINGDEFINES	#XTODO is this correct?

CPPFLAGS.CvtStdSel.c=	${X11FLAGS.CONNECTION}
CPPFLAGS.LocBitmap.c=	-DBITMAPDIR=\"${X11INCDIR}/X11/bitmaps\"

#LM#	LDADD+=	-lXt -lSM -lICE -lXext -lX11
#LM#	DPADD+=	${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.include <bsd.lib.mk>
.include <bsd.x11.mk>
