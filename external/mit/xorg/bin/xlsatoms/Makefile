#	$NetBSD: Makefile,v 1.2.6.1 2014/05/22 14:09:38 yamt Exp $

.include <bsd.own.mk>

PROG=	xlsatoms

LDADD+=	-lxcb
DPADD+=	${LIBXCB}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
