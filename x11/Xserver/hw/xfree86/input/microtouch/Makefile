#	$NetBSD: Makefile,v 1.3.4.1 2004/11/15 09:12:42 rtr Exp $

XMODULE=	microtouch

.include "../../../../Makefile.serverlib"
.include "Makefile.microtouch"

SRCS+=		${SRCS.microtouch}
CPPFLAGS+=	${CPPFLAGS.microtouch} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/input
.include "../../Makefile.drivermod"
