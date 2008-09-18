#	$NetBSD: Makefile,v 1.1.2.2 2008/09/18 05:18:46 wrstuden Exp $

PROG=	cvt
SRCS=	cvt.c xf86cvt.c
MAN=	cvt.1

.include "../Makefile.utils"

CPPFLAGS+=	${X11FLAGS.DIX} -DHAVE_XORG_CONFIG_H \
		-I${DESTDIR}${X11INCDIR}/xorg \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${X11SRCDIR.xorg-server}/../include

LDADD+=	${LIBDUMMY}
DPADD+=	${LIBDUMMY}

# XXX
cvt.1: cvt.man.pre
	cp ${.ALLSRC} ${.TARGET}

.PATH: ${X11SRCDIR.xorg-server}/hw/xfree86/utils/${PROG}
.PATH: ${X11SRCDIR.xorg-server}/hw/xfree86/common

.include <bsd.x11.mk>
.include <bsd.prog.mk>
