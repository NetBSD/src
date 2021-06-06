#	$NetBSD: Makefile,v 1.3.4.1 2021/06/06 20:30:46 cjep Exp $

.include <bsd.own.mk>

CPPFLAGS+=-DXRENDER

PROG=	xeyes
SRCS=	xeyes.c Eyes.c transform.c

LDADD+=	${XLIBLDADD_XMU} ${XLIBLDADD_XRENDER} -lm
DPADD+=	${XLIBDPADD_XMU} ${XLIBDPADD_XRENDER} ${LIBM}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
