#	$NetBSD: Makefile,v 1.13 2004/01/06 02:10:43 lukem Exp $

.include <bsd.own.mk>

SUBDIR=		dbsym mdsetimage

.if ${MACHINE} == "ibmnws"
SUBDIR+=	ncdcs
.endif

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
