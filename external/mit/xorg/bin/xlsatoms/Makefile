#	$NetBSD: Makefile,v 1.2.12.1 2013/06/23 06:28:41 tls Exp $

.include <bsd.own.mk>

PROG=	xlsatoms

LDADD+=	-lxcb
DPADD+=	${LIBXCB}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
