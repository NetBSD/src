#	$NetBSD: Makefile,v 1.3.4.1 2004/11/15 09:12:41 rtr Exp $

XMODULE=	dmc

.include "../../../../Makefile.serverlib"
.include "Makefile.dmc"

SRCS+=		${SRCS.dmc}
CPPFLAGS+=	${CPPFLAGS.dmc} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/input
.include "../../Makefile.drivermod"
