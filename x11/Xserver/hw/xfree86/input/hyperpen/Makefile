#	$NetBSD: Makefile,v 1.2.4.1 2004/11/15 09:12:41 rtr Exp $

XMODULE=	hyperpen

.include "../../../../Makefile.serverlib"
.include "Makefile.hyperpen"

SRCS+=		${SRCS.hyperpen}
CPPFLAGS+=	${CPPFLAGS.hyperpen} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/input
.include "../../Makefile.drivermod"
