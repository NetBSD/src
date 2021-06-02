#	$NetBSD: Makefile,v 1.2.54.1 2021/06/02 14:46:20 cjep Exp $

.include <bsd.own.mk>

PROG=	dga

LDADD+=	${XLIBLDADD_XF86DGA}
DPADD+=	${XLIBDPADD_XF86DGA}

.PATH:	${X11SRCDIR.xf86dga}

COPTS.dga.c+=	-Wno-error	# XXX xf86dga.h

.include <bsd.x11.mk>
.include <bsd.prog.mk>
