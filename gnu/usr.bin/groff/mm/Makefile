#	$NetBSD: Makefile,v 1.14 1998/09/27 17:22:05 lukem Exp $

# XXX -- this depends on ../include/defs.h
TMACDIR?=	/usr/share/tmac

TMACOWN?=	${FILESOWN}
TMACGRP?=	${FILESGRP}
TMACMODE?=	${FILESMODE}

MAN=		groff_mm.7 groff_mmse.7
MLINKS=		groff_mm.7 mm.7 groff_mmse.7 mmse.7

.if !defined(NOSHARE)
FILES=		tmac.m
LINKS=		${TMACDIR}/tmac.m ${TMACDIR}/tmac.mse
FILESDIR=	${TMACDIR}
.endif

SUBDIR=		mm

.include "../Makefile.cfg"
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
