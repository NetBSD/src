#	$NetBSD: Makefile,v 1.1.12.1 2013/06/23 06:28:18 tls Exp $
.include <bsd.own.mk>

SUBDIR=	 libevent
.if ${MKCRYPTO} != "no"
SUBDIR+=	libevent_openssl
.endif
SUBDIR+= libevent_pthreads

.include <bsd.subdir.mk>
