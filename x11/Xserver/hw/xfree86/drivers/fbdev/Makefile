#	$NetBSD: Makefile,v 1.3.4.1 2004/11/15 09:12:40 rtr Exp $

XMODULE=	fbdev

.include "../../../../Makefile.serverlib"
.include "Makefile.fbdev"

SRCS+=		${SRCS.fbdev}
CPPFLAGS+=	${CPPFLAGS.fbdev} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/drivers
.include "../../Makefile.drivermod"
