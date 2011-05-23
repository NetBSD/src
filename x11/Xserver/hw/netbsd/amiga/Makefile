#	$NetBSD: Makefile,v 1.4 2011/05/23 02:37:58 christos Exp $

.include "../../../Makefile.serverlib"

LIB=	amiga

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/amiga
SRCS=	amigaInit.c amigaCfb.c amigaCursor.c amigaFbs.c amigaIo.c \
	amigaKbd.c amigaMfb.c amigaMouse.c amigaKeyMap.c amigaC.c \
	amigaInEx.c

.PATH:		${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/amiga/cirrus
SRCS.cirrus=	amigaCL.c clbitblt.c clfillrct.c clbltC.c clgc.c clsolidC.c clwindow.c

.for _FN in ${SRCS.cirrus}
CPPFLAGS.${_FN}=	-DPSZ=8
.endfor

SRCS+=	${SRCS.cirrus}

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/amiga/retina
SRCS+=	amigaGX.c

.PATH:		${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/amiga/s3
SRCS.s3=	amigaCVscrin.c amigaCV.c amigaCVblt.c amigaCVfrect.c \
		amiga8CVgc.c amiga16CVgc.c amiga32CVgc.c amigaCVim.c \
		amigaCVline.c amigaCVCursor.c amigaCVseg.c amigaCVplypt.c \
		mybcopyas.S amigaCVbstor.c s3bcach.c s3fcach.c s3text.c \
		xf86bcache.c xf86fcache.c xf86text.c s3font.c s3ss.c

.for _FN in ${SRCS.s3}
CPPFLAGS.${_FN}=	-DUSE_IMAGE_WRITE -DUSE_FONTCACHE
.endfor

SRCS+=	${SRCS.s3}

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/amiga \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/amiga/cirrus \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/fb \
		-I${X11SRCDIR.xc}/programs/Xserver/mfb \
		-I${X11SRCDIR.xc}/programs/Xserver/cfb \
		-I${X11SRCDIR.xc}/programs/Xserver/afb \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${NETBSDSRCDIR}/sys/arch/amiga \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${XSERVERINCDIR}

CPPFLAGS+=	-DAMIGAMAXDEPTH=32 -DGFX_CARD_SUPPORT -DAMIGA_CC_COLOR
CPPFLAGS+=	-DCIRRUS_SUPPORT -DRETINAZ3_SUPPORT -DCV64_SUPPORT
#CPPFLAGS+=	-DCV64_SUPPORT -DRETINAZ3_SUPPORT -DCIRRUS_SUPPORT


BUILDSYMLINKS=	../../../mi/miinitext.c amigaInEx.c \
		clblt.c clbltC.c \
		clsolid.c clsolidC.c 

CPPFLAGS.clbltC.c+=	-DMROP=Mcopy
CPPFLAGS.clsolidC.c+=	-DRROP=GXcopy

COPTS.amigaCfb.c=	-Wno-error
COPTS.amigaKbd.c=	-Wno-error

.include <bsd.x11.mk>
.include <bsd.lib.mk>
