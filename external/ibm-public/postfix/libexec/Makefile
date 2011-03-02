#	$NetBSD: Makefile,v 1.3 2011/03/02 22:24:55 tron Exp $

.include <bsd.own.mk>

SUBDIR=	anvil bounce cleanup discard dnsblog error flush local master oqmgr \
	pickup pipe postfix-script postscreen proxymap qmgr scache sendmail \
	showq smtp smtpd spawn trivial-rewrite verify virtual

.if ${MKCRYPTO} != "no"
SUBDIR+=	tlsmgr tlsproxy
.endif

# Not built: fsstone (unneeded), smtpstone (unneeded), qmqpd (unneeded)

.include <bsd.subdir.mk>
