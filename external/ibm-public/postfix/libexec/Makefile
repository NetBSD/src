#	$NetBSD: Makefile,v 1.1 2009/06/25 18:21:53 tron Exp $

.include <bsd.own.mk>

SUBDIR=	anvil bounce cleanup discard error flush local master oqmgr pickup \
	pipe postfix-script proxymap qmgr scache sendmail showq smtp smtpd \
	spawn tlsmgr trivial-rewrite verify virtual

.if ${MKCRYPTO} != "no"
SUBDIR+=	tlsmgr
.endif

# Not built: fsstone (unneeded), smtpstone (unneeded), qmqpd (unneeded)

.include <bsd.subdir.mk>
