#	$NetBSD: Makefile,v 1.2.2.2 2009/09/15 06:01:40 snj Exp $

.include <bsd.own.mk>

.if ${MKPOSTFIX} != "no"
SUBDIR+=	postfix
.endif

# Speedup stubs for some subtrees that don't need to run these rules
includes-postfix:
	@true

.include <bsd.subdir.mk>
