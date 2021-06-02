#	$NetBSD: Makefile,v 1.3.10.1 2021/06/02 14:46:19 cjep Exp $

.include <bsd.own.mk>

PROG=	ico

CPPFLAGS+=-DMULTITHREAD ${X11FLAGS.THREADS} ${X11FLAGS.EXTENSION}

LDADD+=	${XLIBLDADD_XEXT} -lpthread -lm
DPADD+=	${XLIBDPADD_XEXT} ${LIBPTHREAD} ${LIBM}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
