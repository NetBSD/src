#	$NetBSD: Makefile,v 1.1.4.1 2004/11/15 09:12:41 rtr Exp $

XMODULE=	via

.include "../../../../Makefile.serverlib"
.include "Makefile.via"

SRCS+=		${SRCS.via}
CPPFLAGS+=	${CPPFLAGS.via} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/drivers
.include "../../Makefile.drivermod"
