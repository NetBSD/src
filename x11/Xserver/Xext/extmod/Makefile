#	$NetBSD: Makefile,v 1.3 2004/03/06 03:31:05 rtr Exp $

LIB=		extmod
SRCS=	shape.c mitmisc.c bigreq.c sync.c saver.c xcmisc.c xf86vmode.c \
	xf86misc.c xf86dga.c xf86dga2.c cup.c dpms.c EVI.c fontcache.c \
	sampleEVI.c xvmain.c xvdisp.c xvmod.c xvmc.c modinit.c xres.c

CPPFLAGS+=	-DEXTMODULE

XMODULEDIR=	${X11ROOTDIR}/lib/modules/extensions

.include "../../Makefile.serverlib"
.include "../Makefile.ext"

.PATH:          ${X11SRCDIR.xc}/programs/Xserver/Xext/extmod
