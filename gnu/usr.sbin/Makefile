#	$NetBSD: Makefile,v 1.18 2009/11/04 19:02:28 dyoung Exp $

.include <bsd.own.mk>

.if ${MKBINUTILS} != "no" && ${HAVE_BINUTILS} == "216"
SUBDIR=		dbsym mdsetimage ncdcs
.endif

# Speedup stubs for some subtrees that don't need to run these rules
includes-dbsym includes-mdsetimage:
	@true

.include <bsd.subdir.mk>
