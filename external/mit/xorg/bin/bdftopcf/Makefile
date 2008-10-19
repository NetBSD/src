#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:40:56 haad Exp $

.include <bsd.own.mk>

PROG=	bdftopcf

LDADD+=	-lXfont -lfontenc -lfreetype -lm -lz
DPADD+=	${LIBXFONT} ${LIBFONTENC} ${LIBFREETYPE} ${LIBM} ${LIBZ}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
