#	$NetBSD: Makefile,v 1.2 2013/04/11 16:59:16 christos Exp $
.include <bsd.own.mk>

SUBDIR=	 libevent
.if ${MKCRYPTO} != "no"
SUBDIR+=	libevent_openssl
.endif
SUBDIR+= libevent_pthreads

.include <bsd.subdir.mk>
