#	$NetBSD: Makefile,v 1.16 1999/02/13 02:54:27 lukem Exp $

.include <bsd.own.mk>

# XXX -- this depends on ../include/defs.h
TMACDIR?=	/usr/share/tmac

TMACOWN?=	${FILESOWN}
TMACGRP?=	${FILESGRP}
TMACMODE?=	${FILESMODE}

# The Swedish man page is temporarily disabled. We will soon implement
# a scheme for internationalized man pages and turn it on again.
#MAN=		groff_mm.7 groff_mmse.7
#MLINKS= 	groff_mm.7 mm.7 groff_mmse.7 mmse.7
MAN=		groff_mm.7 groff_mmse.7
MLINKS= 	groff_mm.7 mm.7 groff_mmse.7 mmse.7

.if ${MKSHARE} != "no"
FILES=		tmac.m
LINKS=		${TMACDIR}/tmac.m ${TMACDIR}/tmac.mse
FILESDIR=	${TMACDIR}
.endif

SUBDIR=		mm

.include "../Makefile.cfg"
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
