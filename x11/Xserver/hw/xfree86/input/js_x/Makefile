#	$NetBSD: Makefile,v 1.3.4.1 2004/11/15 09:12:41 rtr Exp $

XMODULE=	js_x

.include "../../../../Makefile.serverlib"
.include "Makefile.js_x"

SRCS+=		${SRCS.js_x}
CPPFLAGS+=	${CPPFLAGS.js_x} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/input
.include "../../Makefile.drivermod"
