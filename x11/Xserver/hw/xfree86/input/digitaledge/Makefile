#	$NetBSD: Makefile,v 1.2.4.1 2004/11/15 09:12:41 rtr Exp $

XMODULE=	digitaledge

.include "../../../../Makefile.serverlib"
.include "Makefile.digitaledge"

SRCS+=		${SRCS.digitaledge}
CPPFLAGS+=	${CPPFLAGS.digitaledge} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/input
.include "../../Makefile.drivermod"
