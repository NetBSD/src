#	$NetBSD: Makefile,v 1.2.8.1 2013/06/23 06:28:38 tls Exp $

.include <bsd.own.mk>

PROG=	bdftopcf

CPPFLAGS+=	-I${X11SRCDIR.${PROG}}/../include -DHAVE_CONFIG_H

LDADD+=	-lXfont -lfontenc -lfreetype -lm -lz -lbz2
DPADD+=	${LIBXFONT} ${LIBFONTENC} ${LIBFREETYPE} ${LIBM} ${LIBZ} ${LIBBZ2}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
