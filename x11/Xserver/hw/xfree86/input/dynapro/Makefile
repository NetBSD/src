#	$NetBSD: Makefile,v 1.3.4.1 2004/11/15 09:12:41 rtr Exp $

XMODULE=	dynapro

.include "../../../../Makefile.serverlib"
.include "Makefile.dynapro"

SRCS+=		${SRCS.dynapro}
CPPFLAGS+=	${CPPFLAGS.dynapro} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/input
.include "../../Makefile.drivermod"
