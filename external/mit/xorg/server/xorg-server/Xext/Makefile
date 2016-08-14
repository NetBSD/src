#	$NetBSD: Makefile,v 1.6 2016/08/14 03:43:03 mrg Exp $

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
CPPFLAGS+=	-DHAVE_DIX_CONFIG_H

.if 0
COPTS.panoramiX.c+=		-Wno-error
COPTS.panoramiXprocs.c+=	-Wno-error
COPTS.saver.c+=			-Wno-error
COPTS.security.c+=		-Wno-error
COPTS.shape.c+=			-Wno-error
COPTS.shm.c+=			-Wno-error
COPTS.sync.c+=			-Wno-error
COPTS.xf86bigfont.c+=		-Wno-error
COPTS.xres.c=			-Wno-error
COPTS.xvdisp.c+=		-Wno-error
COPTS.xvmain.c+=		-Wno-error
COPTS.xvmc.c+=			-Wno-error
COPTS.bigreq.c+=		-Wno-error	# XXX bigreqstr.h
COPTS.xcmisc.c+=		-Wno-error	# XXX xcmiscstr.h
.endif

.PATH:		${X11SRCDIR.xorg-server}/Xext

.include <bsd.x11.mk>
.include <bsd.lib.mk>
