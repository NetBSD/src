#	$NetBSD: Makefile,v 1.2.4.1 2004/11/15 09:12:40 rtr Exp $

XMODULE=	s3

.include "../../../../Makefile.serverlib"
.include "Makefile.s3"

SRCS+=		${SRCS.s3}
CPPFLAGS+=	${CPPFLAGS.s3} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/drivers
.include "../../Makefile.drivermod"
