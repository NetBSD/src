#	$Id: Makefile,v 1.1 1997/03/25 21:01:50 christos Exp $

# XXX -- this depends on ../include/defs.h
TMACDIR?=	/usr/share/tmac

TMACOWN?=	bin
TMACGRP?=	bin
TMACMODE?=	444


FILES=		0.MT 5.MT 4.MT ms.cov se_ms.cov locale se_locale
FILESDIR=	${TMACDIR}/mm
FILESOWN=	${TMACOWN}
FILESGRP=	${TMACGRP}
FILESMODE=	${TMACMODE}

CLEANFILES+=locale se_locale

all: locale se_locale
locale:	Makefile
	touch ${.TARGET}

se_locale:	Makefile
	touch ${.TARGET}

.include <bsd.prog.mk>
