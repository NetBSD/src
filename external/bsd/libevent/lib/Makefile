#	$NetBSD: Makefile,v 1.1.12.2 2014/08/19 23:47:16 tls Exp $
.include <bsd.own.mk>

SUBDIR=	 libevent .WAIT
.if ${MKCRYPTO} != "no"
SUBDIR+=	libevent_openssl
.endif
SUBDIR+= libevent_pthreads

.include <bsd.subdir.mk>
