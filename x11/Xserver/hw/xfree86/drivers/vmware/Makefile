#	$NetBSD: Makefile,v 1.3.4.1 2004/11/15 09:12:41 rtr Exp $

XMODULE=	vmware

.include "../../../../Makefile.serverlib"
.include "Makefile.vmware"

SRCS+=		${SRCS.vmware}
CPPFLAGS+=	${CPPFLAGS.vmware} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/drivers
.include "../../Makefile.drivermod"
