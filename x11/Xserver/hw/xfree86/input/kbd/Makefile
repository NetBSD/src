#	$NetBSD: Makefile,v 1.4.4.1 2004/11/15 09:12:41 rtr Exp $

XMODULE=	kbd

.include "../../../../Makefile.serverlib"
.include "Makefile.kbd"

SRCS+=		${SRCS.kbd}
CPPFLAGS+=	${CPPFLAGS.kbd} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/input
.include "../../Makefile.drivermod"
