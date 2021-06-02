#	$NetBSD: Makefile,v 1.3.24.1 2021/06/02 14:46:19 cjep Exp $

.include <bsd.own.mk>

PROG=	fstobdf
SRCS=	chars.c fstobdf.c header.c props.c

LDADD+=	-lFS ${XLIBLDADD_XEXT}
DPADD+=	${LIBFS} ${XLIBDPADD_XEXT}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
