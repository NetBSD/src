#	$NetBSD: Makefile,v 1.4 2010/07/18 11:28:22 mrg Exp $

.include <bsd.own.mk>

PROG=	xlsclients

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR} -DHAVE_STRNLEN

#BUILDSYMLINKS=	xlscli.man xlsclients.man

LDADD+=	-lxcb -lxcb-atom
DPADD+=	${LIBXCB} ${LIBXCB_ATOM}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
