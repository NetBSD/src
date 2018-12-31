#	$NetBSD: Makefile,v 1.9 2018/12/31 11:14:05 mrg Exp $

PROG=	cvt
SRCS=	cvt.c xf86cvt.c xprintf.c
MAN=	cvt.1

.include "../Makefile.utils"

CPPFLAGS+=	${X11FLAGS.DIX} \
		-I${DESTDIR}${X11INCDIR}/xorg \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${X11SRCDIR.xorg-server}/../include \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/ddc \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/modes \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/parser

LDADD+=	-lm
DPADD+=	${LIBM}

.PATH: ${X11SRCDIR.xorg-server}/hw/xfree86/utils/${PROG}
.PATH: ${X11SRCDIR.xorg-server}/hw/xfree86/utils/man
.PATH: ${X11SRCDIR.xorg-server}/hw/xfree86/modes
.PATH: ${X11SRCDIR.xorg-server}/os

.include <bsd.x11.mk>
.include <bsd.prog.mk>
