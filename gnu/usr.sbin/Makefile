#	$NetBSD: Makefile,v 1.14 2005/10/27 17:39:35 jmc Exp $

.include <bsd.own.mk>

SUBDIR=		dbsym mdsetimage ncdcs

.if ${MKPOSTFIX} != "no"
SUBDIR+=	postfix
.endif

.if ${MKSENDMAIL} != "no"
SUBDIR+=	sendmail
.endif

# Speedup stubs for some subtrees that don't need to run these rules
includes-dbsym includes-mdsetimage includes-postfix:
	@true

.include <bsd.subdir.mk>
