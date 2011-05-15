#	$NetBSD: Makefile,v 1.3 2011/05/15 21:35:58 christos Exp $

.include <bsd.own.mk>

PROG=	resize
SRCS=	resize.c xstrings.c

CPPFLAGS+=-I${X11SRCDIR.xterm}
CPPFLAGS+=-DHAVE_TERMCAP_H

.PATH:	${X11SRCDIR.xterm}

LDADD+=	-lterminfo
DPADD+=	${LIBTERMINFO} 

.include <bsd.x11.mk>
.include <bsd.prog.mk>
