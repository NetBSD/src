#	$NetBSD: Makefile,v 1.1 2008/09/01 10:37:19 rtr Exp $

.include <bsd.own.mk>

PROG=	showrgb

CPPFLAGS+=-DNDBM -DRGB_DB=\"${X11LIBDIR}/rgb\"
CPPFLAGS+=-I${X11SRCDIR.xc}/programs/Xserver/include

.PATH:	${X11SRCDIR.xc}/programs/rgb

.include <bsd.x11.mk>
.include <bsd.prog.mk>
