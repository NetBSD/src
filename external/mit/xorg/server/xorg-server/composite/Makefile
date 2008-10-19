#	$NetBSD: Makefile,v 1.2.6.2 2008/10/19 22:41:17 haad Exp $

.include "../Makefile.serverlib"
.include "../Makefile.servermod"

LIB=		composite
.PATH:		${X11SRCDIR.xorg-server}/composite

SRCS=		compalloc.c compext.c compint.h compinit.c compwindow.c

CPPFLAGS+=	${X11FLAGS.PERVASIVE_EXTENSION} ${X11FLAGS.DIX} ${X11INCS.DIX}

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/include \
		-I${X11SRCDIR.xorg-server}/mi \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${X11SRCDIR.xorg-server}/render \
		-I${X11SRCDIR.xorg-server}/xfixes \
		-I${X11SRCDIR.xorg-server}/damageext \
		-I${X11SRCDIR.xorg-server}/miext/damage \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${X11SRCDIR.xorg-server}/../include \

# XXX
COPTS.compwindow.c=	-Wno-error

.include <bsd.x11.mk>
.include <bsd.lib.mk>
