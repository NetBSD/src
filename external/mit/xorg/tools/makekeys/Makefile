#	$NetBSD: Makefile,v 1.6 2018/05/01 17:42:01 christos Exp $

.include <bsd.init.mk>

.PATH:	${X11SRCDIR.X11}/src/util

PROG?=		makekeys
SRCS=		makekeys.c
.if !make(obj) && !exists(${DESTDIR}${X11INCDIR}/X.h)
CLEANFILES+=	X11
CPPFLAGS+=	-I.
XPINCS=		X.h Xarch.h Xos.h Xosdefs.h keysymdef.h Xfuncproto.h
X11INCS=	Xlib.h Xresource.h
CLEANFILES+=	${XPINCS} ${X11INCS}
.if !make(clean) && !make(cleandir)
.BEGIN:
	@rm -f ${.OBJDIR}/X11 && ln -s . X11
.for i in ${XPINCS}
	@rm -f $i && ln -s ${X11SRCDIR.xorgproto}/include/X11/$i .
.endfor
.for i in ${X11INCS}
	@rm -f $i && ln -s ${X11SRCDIR.X11}/include/X11/$i .
.endfor
.endif

.else
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}
.endif

.include <bsd.x11.mk>
.include <bsd.prog.mk>
