#	$NetBSD: Makefile,v 1.11 1999/02/13 02:54:28 lukem Exp $

.include <bsd.own.mk>

# XXX -- this depends on ../include/defs.h
TMACDIR?=	/usr/share/tmac

TMACOWN?=	${FILESOWN}
TMACGRP?=	${FILESGRP}
TMACMODE?=	${FILESMODE}

MAN=		groff_ms.7 msafer.7
MLINKS=		groff_ms.7 ms.7

.if ${MKSHARE} != "no"
FILES=		tmac.pic tmac.ps tmac.psnew tmac.psold tmac.pspic tmac.psatk\
		tmac.dvi tmac.tty tmac.tty-char tmac.X tmac.Xps tmac.latin1\
		tmac.lj4 tmac.safer tmac.a4 tmac.s tmac.an man.local eqnrc \
		troffrc

FILESDIR=	${TMACDIR}
FILESNAME_tmac.an=tmac.an.old
.endif

.include "../Makefile.cfg"
.include <bsd.prog.mk>
