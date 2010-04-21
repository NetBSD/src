#	$NetBSD: Makefile,v 1.2.4.2 2010/04/21 05:23:21 matt Exp $

.include <bsd.own.mk>

.if ${MKPOSTFIX} != "no"
SUBDIR+=	postfix
.endif

# Speedup stubs for some subtrees that don't need to run these rules
includes-postfix:
	@true

.include <bsd.subdir.mk>
