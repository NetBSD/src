#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:06 lukem Exp $

.include <bsd.own.mk>

PROG=	pswrap
SRCS=	main.c pswparser.y lexer.l pswdict.c pswfile.c \
	systemnames.c psw.c pswstring.c pswsemantics.c

YHEADER=1

CPPFLAGS+=-DXENVIRONMENT -DFRIENDSFILE='"<DPS/dpsfriends.h>"' -I.

pswpriv.h: pswparser.h

.PATH:	${X11SRCDIR.xc}/config/pswrap

.include <bsd.prog.mk>
.include <bsd.x11.mk>
