#	$NetBSD: Makefile,v 1.2 2010/07/11 05:37:32 mrg Exp $

.include <bsd.own.mk>

SUBDIR=	anvil bounce cleanup discard error flush local master oqmgr pickup \
	pipe postfix-script proxymap qmgr scache sendmail showq smtp smtpd \
	spawn trivial-rewrite verify virtual

.if ${MKCRYPTO} != "no"
SUBDIR+=	tlsmgr
.endif

# Not built: fsstone (unneeded), smtpstone (unneeded), qmqpd (unneeded)

.include <bsd.subdir.mk>
