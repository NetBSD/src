#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:09 lukem Exp $

NOMAN=		1

.include <bsd.own.mk>

HOSTPROG=	pswrap
.PATH:		${X11SRCDIR.xc}/config/${HOSTPROG}

SRCS=		main.c pswparser.y lexer.l pswdict.c pswfile.c \
		systemnames.c psw.c pswstring.c pswsemantics.c

HOST_CPPFLAGS+=	-DXENVIRONMENT -DFRIENDSFILE='"<DPS/dpsfriends.h>"' -I.

YHEADER=	1

pswpriv.h: pswparser.h

#	Can't use -I${DESTDIR}/${X11INCDIR} since pswrap is needed
#	to create some header files.
#
LNFILES=	${X11SRCDIR.xc}/include X11
DPSRCS+=	X11

.include <bsd.hostprog.mk>
.include <bsd.x11.mk>
