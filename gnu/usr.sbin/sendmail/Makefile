#	$NetBSD: Makefile,v 1.4 2001/01/09 08:52:59 enami Exp $

.include <bsd.own.mk>

SUBDIR=		libsmdb libsmutil .WAIT \
		    sendmail mailstats makemap praliases smrsh

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
