#	$NetBSD: Makefile,v 1.1.1.1.2.2 2008/09/18 05:18:25 wrstuden Exp $

.include <bsd.own.mk>

PROG=	bdftopcf

LDADD+=	-lXfont -lfontenc -lfreetype -lm -lz
DPADD+=	${LIBXFONT} ${LIBFONTENC} ${LIBFREETYPE} ${LIBM} ${LIBZ}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
