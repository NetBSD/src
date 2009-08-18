#	$NetBSD: Makefile,v 1.17 2009/08/18 15:11:25 skrll Exp $

.include <bsd.own.mk>

.if ${HAVE_BINUTILS} == "216"
SUBDIR=		dbsym mdsetimage ncdcs
.endif

# Speedup stubs for some subtrees that don't need to run these rules
includes-dbsym includes-mdsetimage:
	@true

.include <bsd.subdir.mk>
