#	$NetBSD: Makefile,v 1.6.2.2 2004/11/21 10:01:28 rtr Exp $

.include "../Makefile.serverlib"
.include "../Makefile.servermod"

SUBDIR=		raster pcl ps pcl-mono

LIB=		printer

.PATH:		${X11SRCDIR.xc}/programs/Xserver/Xprint
SRCS=		Init.c spooler.c attributes.c Util.c mediaSizes.c \
		Oid.c AttrValid.c ddxInit.c

.PATH:		${X11SRCDIR.xc}/lib/X11
SRCS+=		Quarks.c

.PATH:		${X11SRCDIR.xc}/programs/Xserver/mi
SRCS+=		miinitext.c

.PATH:		${X11SRCDIR.xc}/programs/Xserver/Xext
SRCS+=		dpmsstubs.c


CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/programs/Xserver/mfb \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/cfb \
		-I${X11SRCDIR.xc}/include \
		-I${X11SRCDIR.xc}/programs/Xserver/Xprint \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/lib/X11 \
		-I${DESTDIR}${X11INCDIR}/X11/extensions

CPPFLAGS+=	-DXPRINTDIR=\"${X11ETCDIR}/xserver\" \
		-DXPRASTERDDX -DXPPCLDDX -DXPPSDDX -DHAS_MKSTEMP \
		-UXFree86LOADER -D_XP_PRINT_SERVER_ -DXPMONOPCLDDX

CPPFLAGS.ddxInit.c=	${X11FLAGS.OS_DEFINES}


.include "../Makefile.common"

.if ${XSERVER_XFREE86} == "no"
BUILDCPPFLAGS+=		-UDDXTIME
.endif

.include <bsd.x11.mk>
.include <bsd.lib.mk>
.include <bsd.subdir.mk>
