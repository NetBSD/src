#	$NetBSD: Makefile,v 1.2 2010/07/18 10:58:06 mrg Exp $

.include <bsd.own.mk>

PROG=	xlsclients

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}

#BUILDSYMLINKS=	xlscli.man xlsclients.man

LDADD+=	-lxcb
DPADD+=	${LIBXCB}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
