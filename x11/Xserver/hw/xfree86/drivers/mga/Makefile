#	$NetBSD: Makefile,v 1.5.4.1 2004/11/15 09:12:40 rtr Exp $

XMODULE=	mga

.include "../../../../Makefile.serverlib"
.include "Makefile.mga"

SRCS+=		${SRCS.mga}
CPPFLAGS+=	${CPPFLAGS.mga} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/drivers
.include "../../Makefile.drivermod"
