#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:08 lukem Exp $

XTODO=	InstallDriverSDKNonExecFile

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.xc}/include

INCSDIR=${X11INCDIR}/X11

INCS=	DECkeysym.h HPkeysym.h Sunkeysym.h X.h XF86keysym.h XWDFile.h \
	Xalloca.h Xarch.h Xatom.h Xdefs.h Xfuncproto.h Xfuncs.h Xmd.h Xos.h \
	Xos_r.h Xosdefs.h Xpoll.h Xproto.h Xprotostr.h Xthreads.h ap_keysym.h \
	keysym.h keysymdef.h

SUBDIR=	DPS GL bitmaps extensions fonts

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
.include <bsd.x11.mk>
