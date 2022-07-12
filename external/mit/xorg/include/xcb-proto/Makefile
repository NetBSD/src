#	$NetBSD: Makefile,v 1.6 2022/07/12 02:43:46 mrg Exp $

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.xcb-proto} ${X11SRCDIR.xcb-proto}/src

# dbe.xml

FILES=	bigreq.xml \
	composite.xml \
	damage.xml dbe.xml dpms.xml dri2.xml dri3.xml \
	ge.xml glx.xml \
	present.xml \
	randr.xml record.xml render.xml res.xml \
	screensaver.xml shape.xml shm.xml sync.xml \
	xc_misc.xml xcb.xsd \
	xf86dri.xml xf86vidmode.xml \
	xfixes.xml \
	xinerama.xml xinput.xml \
	xkb.xml \
	xproto.xml \
	xselinux.xml \
	xtest.xml \
	xv.xml xvmc.xml
FILESDIR=${X11SHAREDIR}/xcb

PKGCONFIG=	xcb-proto
PKGCONFIG_SED_FLAGS= \
	-e "s,@PYTHON_PREFIX@,," \
	-e "s,@pythondir@,,"

NOPROG=	# defined

.include <bsd.x11.mk>
.include <bsd.files.mk>
.include <bsd.prog.mk>
