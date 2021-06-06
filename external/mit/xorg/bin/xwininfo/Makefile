#	$NetBSD: Makefile,v 1.7.6.1 2021/06/06 20:30:47 cjep Exp $

.include <bsd.own.mk>

PROG=	xwininfo
SRCS=	xwininfo.c clientwin.c dsimple.c

XWININFODIR=	${X11SRCDIR.${PROG}}
CPPFLAGS+=-I${XWININFODIR} -I${XWININFODIR}/../include -DICONV_CONST=''

.PATH:	${XWININFODIR} ${XWININFODIR}/man

LDADD+=	${XLIBLDADD_XCB_SHAPE} ${XLIBLDADD_XMUU}
DPADD+=	${XLIBDPADD_XCB_SHAPE} ${XLIBDPADD_XMUU}

CWARNFLAGS.clang+=	-Wno-tautological-compare

.include <bsd.x11.mk>
.include <bsd.prog.mk>
