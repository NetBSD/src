#	$NetBSD: Makefile,v 1.3.4.1 2004/11/15 09:12:38 rtr Exp $

LIB=		extmod

SRCS=	shape.c mitmisc.c bigreq.c sync.c saver.c xcmisc.c xf86vmode.c \
	xf86misc.c xf86dga.c xf86dga2.c cup.c dpms.c EVI.c sampleEVI.c \
	xvmain.c xvdisp.c xvmod.c xvmc.c xres.c modinit.c

CPPFLAGS+=	-DEXTMODULE -DXORG_VERSION_CURRENT=${XORG_VERSION_CURRENT}

XMODULEDIR=	${X11ROOTDIR}/lib/modules/extensions

.include "../../Makefile.serverlib"
.include "../Makefile.ext"

.PATH:          ${X11SRCDIR.xc}/programs/Xserver/Xext/extmod
