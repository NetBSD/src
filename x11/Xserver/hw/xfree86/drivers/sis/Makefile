#	$NetBSD: Makefile,v 1.3.4.1 2004/11/15 09:12:40 rtr Exp $

XMODULE=	sis

.include "../../../../Makefile.serverlib"
.include "Makefile.sis"

SRCS+=		${SRCS.sis}
CPPFLAGS+=	${CPPFLAGS.sis} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/drivers
.include "../../Makefile.drivermod"
