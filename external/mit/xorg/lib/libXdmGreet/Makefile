#	$NetBSD: Makefile,v 1.6 2019/03/06 02:44:05 christos Exp $

.include <bsd.own.mk>

LIB=	XdmGreet
.PATH:	${X11SRCDIR.xdm}/greeter

SRCS=	\
	Login.c \
	greet.c \
	verify.c

CPPFLAGS+=	-I${X11SRCDIR.xdm}/include -DRETSIGTYPE=void
CPPFLAGS+=	-DHAVE_SECURITY_PAM_TYPES_H -DHAVE_SECURITY_PAM_APPL_H
# XXX: Enabling this, which should allow --export-dynamic to
# be elided, causes xdm to fail to accept password input and
# then crash at login "failure".
#CPPFLAGS+=	-DGREET_LIB
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/freetype2/freetype

LIBDPLIBS=\
	Xext	${.CURDIR}/../libXext \
	Xt	${.CURDIR}/../libXt \
	Xdmcp	${.CURDIR}/../libXdmcp \
	Xinerama	${.CURDIR}/../libXinerama \
	Xft	${.CURDIR}/../libXft \
	Xpm	${.CURDIR}/../libXpm \
	X11	${.CURDIR}/../libX11/dynamic

.if ${USE_PAM} != "no"
LIBDPLIBS+=\
	pam	${.CURDIR}/../../../../../lib/libpam/libpam
.endif

LIBDPLIBS+=\
	crypt	${.CURDIR}/../../../../../lib/libcrypt \
	util	${.CURDIR}/../../../../../lib/libutil

.include "../../bin/xdm/Makefile.xdm"

.include <bsd.x11.mk>
.include <bsd.lib.mk>
