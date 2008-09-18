#	$NetBSD: Makefile,v 1.1.2.2 2008/09/18 05:18:46 wrstuden Exp $

.include <bsd.own.mk>

PROG=	gtf
MAN=	gtf.1

CPPFLAGS+=	-DHAVE_XORG_CONFIG_H \
		-I${X11SRCDIR.xorg-server}/../include

LDADD+=	-lm
DPADD+=	${LIBM}

.PATH: ${X11SRCDIR.xorg-server}/hw/xfree86/utils/${PROG}

# XXX
gtf.1: gtf.man.pre
	cp ${.ALLSRC} ${.TARGET}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
