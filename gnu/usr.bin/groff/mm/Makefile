#	$Id: Makefile,v 1.10 1997/03/24 22:06:31 christos Exp $

# XXX -- this depends on ../include/defs.h
TMACDIR?=	/usr/share/tmac

TMACOWN?=	bin
TMACGRP?=	bin
TMACMODE?=	444

MAN=		groff_mm.7 groff_mmse.7
MLINKS=		groff_mm.7 mm.7 groff_mmse.7 mmse.7

FILES=		tmac.m
LINKS=		${TMACDIR}/tmac.m ${TMACDIR}/tmac.mse
FILESDIR=	${TMACDIR}
FILESOWN=	${TMACOWN}
FILESGRP=	${TMACGRP}
FILESMODE=	${TMACMODE}

SUBDIR=		mm

.include "../Makefile.cfg"
.include <bsd.prog.mk>
