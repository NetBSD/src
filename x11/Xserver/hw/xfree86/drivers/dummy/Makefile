#	$NetBSD: Makefile,v 1.2.4.1 2004/11/15 09:12:40 rtr Exp $

XMODULE=	dummy

.include "../../../../Makefile.serverlib"
.include "Makefile.dummy"

SRCS+=		${SRCS.dummy}
CPPFLAGS+=	${CPPFLAGS.dummy} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/drivers
.include "../../Makefile.drivermod"
