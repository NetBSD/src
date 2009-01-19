#	$NetBSD: Makefile,v 1.2 2009/01/19 08:02:16 jmmv Exp $

.include <bsd.own.mk>

FILESDIR=	/usr/share/xml
FILESMODE=	444
# XXX Huge hack for the lack of catalog management.  However, this will do
# the trick for now (at least until we have other catalogs to deal with).
.if (${MKATF} != "no")
FILES=			catalog.atf
FILESNAME_catalog.atf=	catalog
.else
FILES=			catalog
.endif

.include <bsd.files.mk>
