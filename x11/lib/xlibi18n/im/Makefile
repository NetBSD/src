#	$NetBSD: Makefile,v 1.1.1.1 2003/09/12 02:52:32 lukem Exp $

LIB=	ximcp
SRCS=	imCallbk.c imDefFlt.c imDefIc.c imDefIm.c imDefLkup.c \
	imDispch.c imEvToWire.c imExten.c imImSw.c imInsClbk.c \
	imInt.c imLcFlt.c imLcGIc.c imLcIc.c imLcIm.c imLcLkup.c \
	imLcPrs.c imLcSIc.c imRm.c imRmAttr.c imThaiFlt.c imThaiIc.c \
	imThaiIm.c imTrX.c imTrans.c imTransR.c

CPPFLAGS.imTrans.c=	${X11FLAGS.CONNECTION} -DXIM_t -DTRANS_CLIENT
CPPFLAGS.imTransR.c=	${X11FLAGS.CONNECTION} -DXIM_t -DTRANS_CLIENT

.include "../Makefile.xlibi18n"
