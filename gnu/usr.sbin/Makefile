#	$NetBSD: Makefile,v 1.19.6.2 2012/11/20 18:53:19 matt Exp $

.include <bsd.own.mk>

.if ${MKBINUTILS} != "no" && ${HAVE_BINUTILS} == "216"
SUBDIR=		dbsym mdsetimage ncdcs
.endif

# Speedup stubs for some subtrees that don't need to run these rules
includes-dbsym includes-mdsetimage:
	@true

.include <bsd.subdir.mk>
