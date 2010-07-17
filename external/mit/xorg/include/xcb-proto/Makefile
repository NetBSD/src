#	$NetBSD: Makefile,v 1.1 2010/07/17 23:44:29 mrg Exp $

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.xcb-proto} ${X11SRCDIR.xcb-proto}/src

FILES=	bigreq.xml composite.xml damage.xml dpms.xml dri2.xml \
	glx.xml randr.xml record.xml render.xml res.xml \
	screensaver.xml shape.xml shm.xml sync.xml xc_misc.xml \
	xcb.xsd xevie.xml xf86dri.xml xfixes.xml xinerama.xml \
	xinput.xml xprint.xml xproto.xml xselinux.xml xtest.xml \
	xv.xml xvmc.xml
FILESDIR=${X11SHAREDIR}/xcb

PKGCONFIG=	xcb-proto

NOPROG=	# defined

.include <bsd.x11.mk>
.include <bsd.files.mk>
.include <bsd.prog.mk>
