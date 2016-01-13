# $NetBSD: Makefile,v 1.1 2016/01/13 19:02:01 christos Exp $

SUBDIR=		examples momdoc
DIST_SUBDIR=    contrib/mom

.include <bsd.init.mk>

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
