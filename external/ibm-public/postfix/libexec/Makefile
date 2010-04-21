#	$NetBSD: Makefile,v 1.1.4.2 2010/04/21 05:24:28 matt Exp $

.include <bsd.own.mk>

SUBDIR=	anvil bounce cleanup discard error flush local master oqmgr pickup \
	pipe postfix-script proxymap qmgr scache sendmail showq smtp smtpd \
	spawn tlsmgr trivial-rewrite verify virtual

.if ${MKCRYPTO} != "no"
SUBDIR+=	tlsmgr
.endif

# Not built: fsstone (unneeded), smtpstone (unneeded), qmqpd (unneeded)

.include <bsd.subdir.mk>
