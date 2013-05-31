#	$NetBSD: Makefile,v 1.2 2013/05/31 05:24:49 mrg Exp $

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.mkfontdir}
.PATH:	${X11SRCDIR.mkfontdir}/man

MAN=	mkfontdir.1

CPPSCRIPTS=	mkfontdir
CPPSCRIPTFLAGS=	-DBINDIR=${X11BINDIR}
SCRIPTS=	mkfontdir

.include <bsd.x11.mk>
.include <bsd.prog.mk>
