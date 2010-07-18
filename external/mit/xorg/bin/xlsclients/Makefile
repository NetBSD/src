#	$NetBSD: Makefile,v 1.3 2010/07/18 11:21:43 mrg Exp $

.include <bsd.own.mk>

PROG=	xlsclients

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}

#BUILDSYMLINKS=	xlscli.man xlsclients.man

LDADD+=	-lxcb -lxcb-atom
DPADD+=	${LIBXCB} ${LIBXCB_ATOM}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
