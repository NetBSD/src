#	$NetBSD: Makefile,v 1.8 1997/03/24 22:06:33 christos Exp $

# XXX -- this depends on ../include/defs.h
TMACDIR?=	/usr/share/tmac

TMACOWN?=	bin
TMACGRP?=	bin
TMACMODE?=	444

MAN=		groff_ms.7 msafer.7
MLINKS=		groff_ms.7 ms.7

FILES=		tmac.pic tmac.ps tmac.psnew tmac.psold tmac.pspic tmac.psatk\
		tmac.dvi tmac.tty tmac.tty-char tmac.X tmac.Xps tmac.latin1\
		tmac.lj4 tmac.safer tmac.a4 tmac.s tmac.an man.local eqnrc \
		troffrc

FILESOWN=	${TMACOWN}
FILESGRP=	${TMACGRP}
FILESMODE=	${TMACMODE}
FILESDIR=	${TMACDIR}
FILESNAME_tmac.an=tmac.an.old


.include "../Makefile.cfg"
.include <bsd.prog.mk>
