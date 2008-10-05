#	$NetBSD: Makefile,v 1.1 2008/10/05 17:35:13 cube Exp $

.include <bsd.own.mk>

PROG=	xmlwf

.PATH:	${X11SRCDIR.expat}/xmlwf ${X11SRCDIR.expat}/doc

SRCS=	codepage.c unixfilemap.c xmlfile.c xmlwf.c

LDADD=	-lexpat

.include <bsd.x11.mk>
.include <bsd.prog.mk>
