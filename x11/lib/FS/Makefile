#	$NetBSD: Makefile,v 1.6 2004/04/05 12:03:25 lukem Exp $

.include <bsd.own.mk>

LIB=	FS
.PATH:	${X11SRCDIR.xc}/lib/${LIB}
.PATH:	${X11SRCDIR.xc}/lib/xtrans

SRCS=	FSClServ.c FSCloseFt.c FSConnServ.c FSErrDis.c FSErrHndlr.c \
	FSFlush.c FSFontInfo.c FSFtNames.c FSGetCats.c FSListCats.c \
	FSListExt.c FSMisc.c FSNextEv.c FSOpenFont.c FSOpenServ.c \
	FSQGlyphs.c FSQXExt.c FSQXInfo.c FSQuExt.c FSServName.c \
	FSSetCats.c FSSync.c FSSynchro.c FSlibInt.c transport.c

INCS=	FSlib.h
INCSDIR=${X11INCDIR}/X11/fonts

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/fonts
CPPFLAGS+=	-I${X11SRCDIR.xc}/lib/${LIB}
CPPFLAGS+=	-I${X11SRCDIR.xc}/lib/xtrans

CONN_DEFINES = ${X11FLAGS.CONNECTION} -DFS_t -DTRANS_CLIENT
SOCK_DEFINES = -DBSD44SOCKETS

CPPFLAGS.FSlibInt.c=	$(CONN_DEFINES)
CPPFLAGS.FSOpenServ.c=	$(CONN_DEFINES)
CPPFLAGS.FSConnServ.c=	$(CONN_DEFINES) $(SOCK_DEFINES)
CPPFLAGS.transport.c=	$(CONN_DEFINES) $(SOCK_DEFINES)

COPTS+=			-Wno-error	# XXX gcc 3.3

.include <bsd.x11.mk>
.include <bsd.lib.mk>
