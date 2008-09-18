#	$NetBSD: Makefile,v 1.1.2.2 2008/09/18 04:54:20 wrstuden Exp $

.include <bsd.own.mk>

PROG=	showrgb

CPPFLAGS+=-DNDBM -DRGB_DB=\"${X11LIBDIR}/rgb\"
CPPFLAGS+=-I${X11SRCDIR.xc}/programs/Xserver/include

.PATH:	${X11SRCDIR.xc}/programs/rgb

.include <bsd.x11.mk>
.include <bsd.prog.mk>
