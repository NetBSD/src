#	$NetBSD: Makefile,v 1.10 2003/07/04 04:53:50 atatat Exp $

# XXX Temporary for NO_SENDMAIL
.include <bsd.own.mk>

SUBDIR=		dbsym mdsetimage postfix

.if !defined(NO_SENDMAIL) || ${NO_SENDMAIL} == "no"
SUBDIR+=	sendmail
.endif

# Speedup stubs for some subtrees that don't need to run these rules
includes-dbsym includes-mdsetimage includes-postfix:
	@true

.include <bsd.subdir.mk>
