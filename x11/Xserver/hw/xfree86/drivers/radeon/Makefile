#	$NetBSD: Makefile,v 1.7.4.1 2004/11/15 09:12:40 rtr Exp $

XMODULE=	radeon

.include "../../../../Makefile.serverlib"
.include "Makefile.radeon"

SRCS+=		${SRCS.radeon}
CPPFLAGS+=	${CPPFLAGS.radeon} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/drivers
.include "../../Makefile.drivermod"
