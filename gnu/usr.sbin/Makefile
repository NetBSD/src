#	$NetBSD: Makefile,v 1.12 2003/10/19 03:34:59 matt Exp $

.include <bsd.own.mk>

SUBDIR=		dbsym mdsetimage

.if ${MACHINE} == "ibmnws"
SUBDIR+=	ncdcs
.endif

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
