#	$NetBSD: Makefile,v 1.2.4.1 2004/11/15 09:12:39 rtr Exp $

XMODULE=	ark

.include "../../../../Makefile.serverlib"
.include "Makefile.ark"

SRCS+=		${SRCS.ark}
CPPFLAGS+=	${CPPFLAGS.ark} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/drivers
.include "../../Makefile.drivermod"
