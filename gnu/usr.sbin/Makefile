#	$NetBSD: Makefile,v 1.11 2003/10/18 23:46:26 matt Exp $

.include <bsd.own.mk>

SUBDIR=		dbsym mdsetimage

.if ${MKPOSTFIX} != "no"
SUBDIR+=	postfix
.endif

.if (!defined(NO_SENDMAIL) || ${NO_SENDMAIL} == "no")
.if ${MKSENDMAIL} != "no"
SUBDIR+=	sendmail
.endif
.endif

# Speedup stubs for some subtrees that don't need to run these rules
includes-dbsym includes-mdsetimage includes-postfix:
	@true

.include <bsd.subdir.mk>
