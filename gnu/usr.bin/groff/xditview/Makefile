#	$Id: Makefile,v 1.3 1993/08/02 17:45:54 mycroft Exp $

.if exists(/usr/X386)

BINDIR=		/usr/X386/bin
MANDIR=		/usr/X386/man/cat

PROG=		gxditview
CFLAGS+=	-I/usr/X386/include -DFONTPATH=\"/usr/share/groff_font\"
SRCS+=		xditview.c Dvi.c draw.c font.c lex.c page.c parse.c \
		XFontName.c DviChar.c device.c
LDADD+=		-L/usr/X386/lib -lXaw -lXmu -lXt -lXext -lX11 -lm 

afterinstall:
		install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
			${.CURDIR}/GXditview.ad \
			$(DESTDIR)/usr/X386/lib/X11/app-defaults
.endif

.include <bsd.prog.mk>
