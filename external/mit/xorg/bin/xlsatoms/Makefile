#	$NetBSD: Makefile,v 1.2 2010/07/18 10:42:24 mrg Exp $

.include <bsd.own.mk>

PROG=	xlsatoms

LDADD+=	-lxcb
DPADD+=	${LIBXCB}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
