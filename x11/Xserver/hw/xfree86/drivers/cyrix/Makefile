#	$NetBSD: Makefile,v 1.3.4.1 2004/11/15 09:12:40 rtr Exp $

XMODULE=	cyrix

.include "../../../../Makefile.serverlib"
.include "Makefile.cyrix"

SRCS+=		${SRCS.cyrix}
CPPFLAGS+=	${CPPFLAGS.cyrix} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/drivers
.include "../../Makefile.drivermod"
