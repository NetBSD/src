# $NetBSD: Makefile,v 1.2 2006/02/12 18:11:54 tron Exp $

SUBDIR=		examples momdoc
DIST_SUBDIR=    contrib/mom

TMACOWN?=	${BINOWN}
TMACGRP?=	${BINGRP}
TMACMODE?=	${NOBINMODE}
# XXX: next depends on ../src/include/defs.h
TMACDIR?=	/usr/share/tmac

NORMALFILES=	mom.tmac om.tmac
MAN=		groff_mom.7

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILESDIR=	${TMACDIR}
FILES+=		${NORMALFILES}
.endif

CLEANFILES+=	groff_mom.7

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
