#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:06 lukem Exp $

.include <bsd.own.mk>

PROG=	mkfontscale
SRCS=	mkfontscale.c list.c

CPPFLAGS+=-DFREETYPE2 -I${DESTDIR}/${X11INCDIR}/freetype2

LDADD+=	-lfontenc -lfreetype -lz
DPADD+=	${LIBFONTENC} ${LIBFREETYPE} ${LIBZ}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.prog.mk>
.include <bsd.x11.mk>
