#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:38 lukem Exp $

.include <bsd.own.mk>

SUBDIR=	xtrapinfo xtrapout xtrapreset xtrapstats xtrapin xtrapproto xtrapchar

MAN=	xtrap.1

.for _M in ${SUBDIR}
MLINKS+=xtrap.1 ${_M}.1
.endfor

.PATH:	${X11SRCDIR.xc}/programs/xtrap

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
