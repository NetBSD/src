#	$NetBSD: Makefile,v 1.3.4.1 2004/11/15 09:12:42 rtr Exp $

XMODULE=	void

.include "../../../../Makefile.serverlib"
.include "Makefile.void"

SRCS+=		${SRCS.void}
CPPFLAGS+=	${CPPFLAGS.void} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/input
.include "../../Makefile.drivermod"
