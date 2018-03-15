#	$NetBSD: Makefile,v 1.2.2.2 2018/03/15 09:11:59 pgoyette Exp $

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.xorgproto}/include/X11

Xpoll.h: Xpoll.h.in
	${TOOL_SED} 's/@USE_FDS_BITS@/fds_bits/' \
	    < ${.ALLSRC} > ${.TARGET}

INCS=	DECkeysym.h \
	HPkeysym.h \
	Sunkeysym.h \
	X.h \
	XF86keysym.h \
	XWDFile.h \
	Xalloca.h \
	Xarch.h \
	Xatom.h \
	Xdefs.h \
	Xfuncproto.h \
	Xfuncs.h \
	Xmd.h \
	Xos.h \
	Xos_r.h \
	Xosdefs.h \
	Xpoll.h \
	Xproto.h \
	Xprotostr.h \
	Xthreads.h \
	Xw32defs.h \
	Xwindows.h \
	Xwinsock.h \
	ap_keysym.h \
	keysym.h \
	keysymdef.h
INCSDIR=${X11INCDIR}/X11

CLEANFILES+=	Xpoll.h

NOOBJ=	# defined

SUBDIR= PM dri extensions fonts

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
