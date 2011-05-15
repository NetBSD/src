#	$NetBSD: Makefile,v 1.3 2011/05/15 21:32:47 christos Exp $

.include <bsd.own.mk>

PROG=	xmlwf

.PATH:	${X11SRCDIR.expat}/xmlwf ${X11SRCDIR.expat}/doc

SRCS=	codepage.c unixfilemap.c xmlfile.c xmlwf.c
CPPFLAGS+=-DHAVE_UNISTD_H

LDADD+=	-lexpat
DPADD+=	${LIBEXPAT}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
