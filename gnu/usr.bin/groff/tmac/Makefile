#	$NetBSD: Makefile,v 1.12 2001/04/19 14:41:43 wiz Exp $

.include "../Makefile.path"

.PATH:		${IDIST}/tmac

.include <bsd.own.mk>

# XXX -- this depends on ../include/defs.h
TMACDIR?=	/usr/share/tmac

TMACOWN?=	${FILESOWN}
TMACGRP?=	${FILESGRP}
TMACMODE?=	${FILESMODE}

# don't install groff_mdoc.7 and groff_mdoc.samples.7, ours are newer
# also don't install groff_me.7, since we're using our own tmac.e and me.7
MAN=		groff_man.7 groff_markup.7 groff_ms.7 groff_msafer.7 \
		groff_tmac.5
MLINKS=		groff_ms.7 ms.7 groff_msafer.7 msafer.7

.if ${MKSHARE} != "no"
FILES=		tmac.pic tmac.ps tmac.psnew tmac.psold tmac.pspic tmac.psatk \
		tmac.dvi tmac.tty tmac.tty-char tmac.X tmac.Xps tmac.latin1 \
		tmac.lj4 tmac.safer tmac.a4 tmac.s tmac.an man.local eqnrc \
		troffrc tmac.arkup tmac.html tmac.trace troffrc-end

FILESDIR=	${TMACDIR}
FILESNAME_tmac.an=tmac.an.old
.endif

.include <bsd.prog.mk>
