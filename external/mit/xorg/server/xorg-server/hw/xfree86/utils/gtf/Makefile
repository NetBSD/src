#	$NetBSD: Makefile,v 1.3.40.1 2019/01/18 08:49:07 pgoyette Exp $

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
