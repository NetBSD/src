#	$NetBSD: Makefile,v 1.3 2003/09/23 11:04:45 lukem Exp $

.include <bsd.own.mk>

PROG=	pswrap
SRCS=	main.c pswparser.y lexer.l pswdict.c pswfile.c \
	systemnames.c psw.c pswstring.c pswsemantics.c

YHEADER=1

CPPFLAGS+=	-DXENVIRONMENT \
		-DFRIENDSFILE='"<DPS/dpsfriends.h>"' \
		-I. \
		-I${X11SRCDIR.xc}/config/pswrap

pswpriv.h: pswparser.h

.PATH:	${X11SRCDIR.xc}/config/pswrap

.include <bsd.x11.mk>
.include <bsd.prog.mk>
