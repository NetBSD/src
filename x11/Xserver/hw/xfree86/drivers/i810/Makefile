#	$NetBSD: Makefile,v 1.3.4.1 2004/11/15 09:12:40 rtr Exp $

XMODULE=	i810

.include "../../../../Makefile.serverlib"
.include "Makefile.i810"

SRCS+=		${SRCS.i810}
CPPFLAGS+=	${CPPFLAGS.i810} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/drivers
.include "../../Makefile.drivermod"
