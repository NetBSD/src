#	$NetBSD: Makefile,v 1.2.2.2 2008/10/19 22:41:20 haad Exp $

.include <bsd.own.mk>

PROG=	gtf
MAN=	gtf.1

CPPFLAGS+=	-DHAVE_XORG_CONFIG_H \
		-I${X11SRCDIR.xorg-server}/../include

LDADD+=	-lm
DPADD+=	${LIBM}

.PATH: ${X11SRCDIR.xorg-server}/hw/xfree86/utils/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
