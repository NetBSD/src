#	$NetBSD: Makefile,v 1.5 2003/03/24 15:04:03 atatat Exp $

.include <bsd.own.mk>

SUBDIR=		libsm libsmdb libsmutil .WAIT \
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
