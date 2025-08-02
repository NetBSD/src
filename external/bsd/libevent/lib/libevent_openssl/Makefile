#	$NetBSD: Makefile,v 1.3.16.1 2025/08/02 05:21:49 perseant Exp $

.include <bsd.own.mk>

LIB=event_openssl

SRCS= \
bufferevent_openssl.c

LIBDPLIBS+= \
	event	${.CURDIR}/../libevent \
	crypto	${NETBSDSRCDIR}/crypto/external/${EXTERNAL_OPENSSL_SUBDIR}/lib/libcrypto \
	ssl	${NETBSDSRCDIR}/crypto/external/${EXTERNAL_OPENSSL_SUBDIR}/lib/libssl

.include <bsd.lib.mk>
