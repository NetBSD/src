#	$NetBSD: Makefile,v 1.3 2015/06/25 01:38:09 matt Exp $

NOMAN=	1

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.X11}/src/util

PROG?=		makekeys
SRCS=		makekeys.c
.if !make(obj) && !exists(${DESTDIR}${X11INCDIR}/X.h)
CLEANFILES+=	X11
CPPFLAGS+=	-I.
XPINCS=		X.h Xarch.h Xos.h Xosdefs.h keysymdef.h
X11INCS=	Xlib.h Xresource.h
CLEANFILES+=	${XPINCS} ${X11INCS} Xfuncproto.h
.BEGIN:
	@rm -f X11 && ln -s . X11
.for i in ${XPINCS}
	@rm -f $i && ln -s ${X11SRCDIR.xproto}/$i .
.endfor
.for i in ${X11INCS}
	@rm -f $i && ln -s ${X11SRCDIR.X11}/include/X11/$i .
.endfor

Xfuncproto.h: ${X11SRCDIR.xproto}/Xfuncproto.h.in
	${TOOL_SED} -e 's/#undef NARROWPROTO/#define NARROWPROTO/'  \
	    -e 's/#undef FUNCPROTO/#define FUNCPROTO 15/' \
	    < ${.ALLSRC} > ${.TARGET}

makekeys.lo makekeys.o makekeys.d: Xfuncproto.h

.else
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}
.endif

.include <bsd.x11.mk>
.include <bsd.prog.mk>
