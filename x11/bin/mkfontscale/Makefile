#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:08 lukem Exp $

.include <bsd.own.mk>

PROG=	mkfontscale
SRCS=	mkfontscale.c list.c

CPPFLAGS+=-DFREETYPE2 -I${DESTDIR}/${X11INCDIR}/freetype2

LDADD+=	-lfontenc -lfreetype -lz
DPADD+=	${LIBFONTENC} ${LIBFREETYPE} ${LIBZ}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
