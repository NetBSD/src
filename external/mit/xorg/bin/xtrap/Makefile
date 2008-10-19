#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:41:04 haad Exp $

.include <bsd.own.mk>

SUBDIR=	xtrapinfo xtrapout xtrapreset xtrapstats xtrapin xtrapproto xtrapchar

MAN=	xtrap.1

.for _M in ${SUBDIR}
MLINKS+=xtrap.1 ${_M}.1
.endfor

.PATH:	${X11SRCDIR.xtrap}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
