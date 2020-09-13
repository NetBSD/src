#	$NetBSD: Makefile,v 1.8 2020/09/13 23:01:02 mrg Exp $

LIBISPRIVATE=	yes
LIB=		Xext

.include "../Makefile.serverlib"

SRCS=	bigreq.c geext.c shape.c sleepuntil.c \
	sync.c xcmisc.c xtest.c
SRCS+=	shm.c
SRCS+=	xvmain.c xvdisp.c xvmc.c
SRCS+=	hashtable.c xres.c
SRCS+=	saver.c
SRCS+=	panoramiX.c panoramiXprocs.c panoramiXSwap.c
SRCS+=	xace.c
SRCS+=	security.c
SRCS+=	xf86bigfont.c
SRCS+=	dpms.c
# XXX not really
SRCS+=	vidmode.c

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/include
CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/Xext
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/pixman-1
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/xorg
CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/Xi
CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/xfixes
CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/miext/sync
CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/composite
CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/damageext
CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/../include ${X11FLAGS.DIX}

.PATH:		${X11SRCDIR.xorg-server}/Xext

.include <bsd.x11.mk>
.include <bsd.lib.mk>
