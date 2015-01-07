#	$NetBSD: Makefile,v 1.2 2015/01/07 20:42:02 joerg Exp $

NOMAN=	1

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.Xt}/util

HOSTPROG=	makestrs
HOST_CPPFLAGS+=	-I.

.for header in Xos.h Xosdefs.h Xarch.h
realdepend realall: X11/${header}

X11/${header}:
	mkdir -p X11
	rm -f ${header}
	ln -s ${X11SRCDIR.xproto}/${header} X11/${header}
.endfor

clean_headers: .PHONY
	rm -rf X11

cleandir: clean_headers

.include <bsd.hostprog.mk>
