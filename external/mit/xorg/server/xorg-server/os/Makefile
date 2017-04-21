#	$NetBSD: Makefile,v 1.14.2.1 2017/04/21 16:53:00 bouyer Exp $

.include "../Makefile.serverlib"
.include "../Makefile.servermod"

LIB=	os

.PATH:	${X11SRCDIR.xorg-server}/os
SRCS=	WaitFor.c access.c auth.c backtrace.c client.c connection.c \
	io.c mitauth.c \
	oscolor.c osinit.c utils.c xdmauth.c \
	xstrans.c xprintf.c log.c xdmcp.c busfault.c \
	rpcauth.c xsha1.c \
	reallocarray.c \
	timingsafe_memcmp.c

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${X11SRCDIR.xorg-server}/render \
		-I${X11SRCDIR.xorg-server}/present \
		-I${X11SRCDIR.xorg-server}/../include

CPPFLAGS+=	${X11FLAGS.DIX} \
		${X11FLAGS.CONNECTION} \
		${X11FLAGS.EXTENSION} \
		-DHASXDMAUTH \
		${X11FLAGS.OS_DEFINES} \
		-DUSE_RGB_TXT \
		-UDDXTIME \
		-DHAVE_XORG_CONFIG_H

.if 0
		-DHAVE_SHA1_IN_LIBC \
		-DHAVE_STRCASECMP -DHAVE_STRNCASECMP \
		-DHAVE_STRCASESTR \
		-DHAVE_REALLOCARRAY \
		-DMONOTONIC_CLOCK
.endif

.include "../Makefile.common"

COPTS.xdmauth.c=	-Wno-error	# XXX
COPTS.xstrans.c=	-Wno-error	# XXX
COPTS.access.c=		-Wno-error	# XXX bigreqstr.h
COPTS.io.c=		-Wno-error	# XXX bigreqstr.h

.include <bsd.x11.mk>
.include <bsd.lib.mk>
