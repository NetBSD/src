#	$NetBSD: Makefile,v 1.4 2013/02/25 00:23:01 jmmv Exp $

.include <bsd.own.mk>

.if !(${MKKYUA} != "no")
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
.endif

.include <bsd.prog.mk> # XXX Bring in missing targets (e.g. includes)
.include <bsd.files.mk>
