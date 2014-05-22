#	$NetBSD: Makefile,v 1.3.8.1 2014/05/22 11:37:54 yamt Exp $

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
