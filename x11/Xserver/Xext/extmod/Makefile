#	$NetBSD: Makefile,v 1.2 2003/12/21 03:07:01 rtr Exp $

LIB=		extmod
SRCS=		modinit.c shape.c mitmisc.c bigreq.c sync.c saver.c xcmisc.c \
		xf86vmode.c xf86misc.c xf86dga.c xf86dga2.c cup.c dpms.c \
		fontcache.c EVI.c sampleEVI.c xvmain.c xvdisp.c xvmod.c \
		xvmc.c xres.c

CPPFLAGS+=	-DEXTMODULE

XMODULEDIR=	${X11ROOTDIR}/lib/modules/extensions

.include "../../Makefile.serverlib"
.include "../Makefile.ext"

.PATH:          ${X11SRCDIR.xc}/programs/Xserver/Xext/extmod
