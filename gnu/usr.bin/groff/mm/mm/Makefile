#	$NetBSD: Makefile,v 1.6 2000/01/23 22:19:27 mycroft Exp $

.include <bsd.own.mk>

# XXX -- this depends on ../include/defs.h
TMACDIR?=	/usr/share/tmac

TMACOWN?=	${FILESOWN}
TMACGRP?=	${FILESGRP}
TMACMODE?=	${FILESMODE}

.if ${MKSHARE} != "no"
FILES=		0.MT 5.MT 4.MT ms.cov se_ms.cov locale se_locale
FILESDIR=	${TMACDIR}/mm
.endif

CLEANFILES+=locale se_locale

realall: locale se_locale
locale:	Makefile
	touch ${.TARGET}

se_locale:	Makefile
	touch ${.TARGET}

.include <bsd.prog.mk>
