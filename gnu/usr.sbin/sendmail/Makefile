#	$NetBSD: Makefile,v 1.6 2003/07/04 04:52:59 atatat Exp $

.include <bsd.own.mk>

SUBDIR=		libsm libsmdb libsmutil libmilter .WAIT \
		    sendmail mailstats makemap editmap praliases smrsh

.if ${MKDOC} != "no"
SUBDIR+=	doc
.endif

.if ${MKSHARE} != "no"
SUBDIR+=	cf/cf
.if make(install)
SUBDIR+=	cf
.endif
.endif

.include <bsd.subdir.mk>
