#	$NetBSD: Makefile,v 1.5.4.1 2004/11/15 09:12:42 rtr Exp $

XMODULE=	mouse

.include "../../../../Makefile.serverlib"
.include "Makefile.mouse"

SRCS+=		${SRCS.mouse}
CPPFLAGS+=	${CPPFLAGS.mouse} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/input
.include "../../Makefile.drivermod"
