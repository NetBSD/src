#	$NetBSD: Makefile,v 1.2 2003/12/21 03:07:01 rtr Exp $

LIB=		ext
SRCS=		EVI.c appgroup.c bigreq.c cup.c dpms.c fontcache.c mitmisc.c \
		panoramiX.c panoramiXSwap.c panoramiXprocs.c sampleEVI.c \
		saver.c security.c shape.c shm.c sleepuntil.c sync.c \
		xcmisc.c xf86bigfont.c xprint.c xres.c xtest.c xtest1dd.c \
		xtest1di.c xvdisp.c xvmain.c xvmc.c

# XXX dga should be enabled for sparc64 but isn't.
.if ${MACHINE} != "sparc" && ${MACHINE} != "sparc64"
SRCS+=	xf86vmode.c xf86misc.c xf86dga.c xf86dga2.c
.endif

FILESDIR=	${X11LIBDIR}/xserver
FILES=		SecurityPolicy

.include "../../Makefile.serverlib"
.include "../Makefile.ext"
