#	$NetBSD: Makefile,v 1.5.2.1 2004/11/15 09:12:39 rtr Exp $

XMODULE=	ati

.include "../../../../Makefile.serverlib"
.include "Makefile.ati"

SRCS+=		${SRCS.ati}
CPPFLAGS+=	${CPPFLAGS.ati} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/drivers
.include "../../Makefile.drivermod"
