#	$NetBSD: Makefile,v 1.3.4.1 2004/11/15 09:12:40 rtr Exp $

XMODULE=	s3virge

.include "../../../../Makefile.serverlib"
.include "Makefile.s3virge"

SRCS+=		${SRCS.s3virge}
CPPFLAGS+=	${CPPFLAGS.s3virge} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/drivers
.include "../../Makefile.drivermod"
