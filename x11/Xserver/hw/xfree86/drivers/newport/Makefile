#	$NetBSD: Makefile,v 1.4.4.1 2004/11/15 09:12:40 rtr Exp $

XMODULE=	newport

.include "../../../../Makefile.serverlib"
.include "Makefile.newport"

SRCS+=		${SRCS.newport}
CPPFLAGS+=	${CPPFLAGS.newport} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/drivers
.include "../../Makefile.drivermod"
