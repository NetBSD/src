#	$NetBSD: Makefile,v 1.4 1998/09/27 17:22:05 lukem Exp $

# XXX -- this depends on ../include/defs.h
TMACDIR?=	/usr/share/tmac

TMACOWN?=	${FILESOWN}
TMACGRP?=	${FILESGRP}
TMACMODE?=	${FILESMODE}

.if !defined(NOSHARE)
FILES=		0.MT 5.MT 4.MT ms.cov se_ms.cov locale se_locale
FILESDIR=	${TMACDIR}/mm
.endif

CLEANFILES+=locale se_locale

all: locale se_locale
locale:	Makefile
	touch ${.TARGET}

se_locale:	Makefile
	touch ${.TARGET}

.include <bsd.prog.mk>
