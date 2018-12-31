#	$NetBSD: Makefile,v 1.4 2018/12/31 11:14:05 mrg Exp $

.include <bsd.own.mk>

PROG=	gtf
MAN=	gtf.1

CPPFLAGS+=	${X11FLAGS.DIX} \
		-I${X11SRCDIR.xorg-server}/../include

LDADD+=	-lm
DPADD+=	${LIBM}

.PATH: ${X11SRCDIR.xorg-server}/hw/xfree86/utils/${PROG}
.PATH: ${X11SRCDIR.xorg-server}/hw/xfree86/utils/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
