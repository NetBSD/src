#	$NetBSD: Makefile,v 1.3.4.1 2004/11/15 09:12:42 rtr Exp $

XMODULE=	tek4957

.include "../../../../Makefile.serverlib"
.include "Makefile.tek4957"

SRCS+=		${SRCS.tek4957}
CPPFLAGS+=	${CPPFLAGS.tek4957} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/input
.include "../../Makefile.drivermod"
