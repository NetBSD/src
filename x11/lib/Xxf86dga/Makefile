#	$NetBSD: Makefile,v 1.4 2004/03/06 03:05:39 rtr Exp $

.include <bsd.own.mk>

LIB=	Xxf86dga
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	XF86DGA.c XF86DGA2.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}

CPPFLAGS.XF86DGA2.c=	-Wno-error	# gcc3

.include <bsd.x11.mk>
.include <bsd.lib.mk>
