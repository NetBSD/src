#	$NetBSD: Makefile,v 1.1.2.2 2004/04/29 05:46:03 jmc Exp $

.include <bsd.own.mk>

PROG=	fonttosfnt
SRCS=	fonttosfnt.c read.c write.c struct.c util.c

CPPFLAGS+=	-DXFREE86_FT2 -I${DESTDIR}${X11INCDIR}/freetype2
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/fonts

LDADD+=	-lfreetype -lfontenc -lz -lm
DPADD+=	 ${LIBFREETYPE} ${LIBFONTENC} ${LIBZ} ${LIBM}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
