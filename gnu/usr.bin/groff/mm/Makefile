#	$NetBSD: Makefile,v 1.17 2001/04/19 14:41:31 wiz Exp $

.include "../Makefile.path"

DIST=		${IDIST}/contrib/mm
.PATH:		${DIST}

.include <bsd.own.mk>

# XXX -- this depends on ../include/defs.h
TMACDIR?=	/usr/share/tmac

TMACOWN?=	${FILESOWN}
TMACGRP?=	${FILESGRP}
TMACMODE?=	${FILESMODE}

# XXX: Following comment seems bogus [wiz 2001-02-25]
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

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
