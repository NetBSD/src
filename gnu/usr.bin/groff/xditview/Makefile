#	$NetBSD: Makefile,v 1.8 1997/10/25 10:12:35 lukem Exp $

.if exists(/usr/X11)
XINCDIR=	/usr/X11/include
XLIBDIR=	/usr/X11/lib
.elif exists(/usr/X11R6)
XINCDIR=	/usr/X11R6/include
XLIBDIR=	/usr/X11R6/lib
.elif exists(/usr/X11R5)
XINCDIR=	/usr/X11R5/include
XLIBDIR=	/usr/X11R5/lib
.elif exists(/usr/X11R4)
XINCDIR=	/usr/X11R4/include
XLIBDIR=	/usr/X11R4/lib
.elif exists(/usr/X386)
XINCDIR=	/usr/X386/include
XLIBDIR=	/usr/X386/lib
.endif

.if defined(XINCDIR) && defined(XLIBDIR)
BINDIR=		/usr/bin

PROG=		gxditview
CPPFLAGS+=	-I$(XINCDIR) -DFONTPATH=\"/usr/share/groff_font\"
SRCS+=		xditview.c Dvi.c draw.c font.c lex.c page.c parse.c \
		XFontName.c DviChar.c device.c
LDADD+=		-L$(XLIBDIR) -lXaw -lXmu -lXt -lXext -lX11 -lm 
DPADD+=		$(LIBM)

afterinstall:
		${INSTALL} -c -o $(BINOWN) -g $(BINGRP) -m 444 \
			$(.CURDIR)/GXditview.ad \
			$(DESTDIR)$(XLIBDIR)/X11/app-defaults/GXditview
.endif

.include <bsd.prog.mk>
