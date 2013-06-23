#	$NetBSD: Makefile,v 1.3.10.1 2013/06/23 06:28:42 tls Exp $

.include <bsd.own.mk>

PROG=	resize
SRCS=	resize.c xstrings.c version.c

CPPFLAGS+=-I${X11SRCDIR.xterm}
CPPFLAGS+=-DHAVE_TERMCAP_H

.PATH:	${X11SRCDIR.xterm}

LDADD+=	-lterminfo
DPADD+=	${LIBTERMINFO} 

.include <bsd.x11.mk>
.include <bsd.prog.mk>
