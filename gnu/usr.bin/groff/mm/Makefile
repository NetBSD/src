#	$NetBSD: Makefile,v 1.13 1998/02/09 04:22:20 lukem Exp $

# XXX -- this depends on ../include/defs.h
TMACDIR?=	/usr/share/tmac

TMACOWN?=	${FILESOWN}
TMACGRP?=	${FILESGRP}
TMACMODE?=	${FILESMODE}

MAN=		groff_mm.7 groff_mmse.7
MLINKS=		groff_mm.7 mm.7 groff_mmse.7 mmse.7

FILES=		tmac.m
LINKS=		${TMACDIR}/tmac.m ${TMACDIR}/tmac.mse
FILESDIR=	${TMACDIR}

SUBDIR=		mm

.include "../Makefile.cfg"
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
