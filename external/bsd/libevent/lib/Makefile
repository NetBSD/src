#	$NetBSD: Makefile,v 1.1.6.1 2014/05/22 15:48:11 yamt Exp $
.include <bsd.own.mk>

SUBDIR=	 libevent .WAIT
.if ${MKCRYPTO} != "no"
SUBDIR+=	libevent_openssl
.endif
SUBDIR+= libevent_pthreads

.include <bsd.subdir.mk>
