#	$NetBSD: Makefile,v 1.4.2.1 2004/11/15 09:12:41 rtr Exp $

XMODULE=	trident

.include "../../../../Makefile.serverlib"
.include "Makefile.trident"

SRCS+=		${SRCS.trident}
CPPFLAGS+=	${CPPFLAGS.trident} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/drivers
.include "../../Makefile.drivermod"
