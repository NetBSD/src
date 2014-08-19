#	$NetBSD: Makefile,v 1.1.12.2 2014/08/20 00:00:10 tls Exp $

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.xcb-proto} ${X11SRCDIR.xcb-proto}/src

FILES=	bigreq.xml composite.xml damage.xml dpms.xml dri2.xml dri3.xml \
	ge.xml glx.xml present.xml randr.xml record.xml render.xml \
	res.xml screensaver.xml shape.xml shm.xml sync.xml xc_misc.xml \
	xcb.xsd xevie.xml xf86dri.xml xf86vidmode.xml xfixes.xml \
	xinerama.xml xkb.xml \
	xinput.xml xprint.xml xproto.xml xselinux.xml xtest.xml \
	xv.xml xvmc.xml
FILESDIR=${X11SHAREDIR}/xcb

PKGCONFIG=	xcb-proto
PKGCONFIG_SED_FLAGS= \
	-e "s,@pythondir@,,"

NOPROG=	# defined

.include <bsd.x11.mk>
.include <bsd.files.mk>
.include <bsd.prog.mk>
