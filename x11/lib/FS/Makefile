#	$NetBSD: Makefile,v 1.1.1.1 2003/09/12 02:52:17 lukem Exp $

NOPIC=	1

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

CPPFLAGS+=	-I${DESTDIR}/${X11INCDIR}/X11/fonts

CONN_DEFINES = ${X11FLAGS.CONNECTION} -DFS_t -DTRANS_CLIENT
SOCK_DEFINES = -DBSD44SOCKETS

CPPFLAGS.FSlibInt.c=	$(CONN_DEFINES)
CPPFLAGS.FSOpenServ.c=	$(CONN_DEFINES)
CPPFLAGS.FSConnServ.c=	$(CONN_DEFINES) $(SOCK_DEFINES)
CPPFLAGS.transport.c=	$(CONN_DEFINES) $(SOCK_DEFINES)

.include <bsd.lib.mk>
.include <bsd.x11.mk>
