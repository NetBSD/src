#	$NetBSD: Makefile,v 1.1.4.3 2014/08/19 23:47:16 tls Exp $

.include <bsd.own.mk>

LIB=event_openssl

SRCS= \
bufferevent_openssl.c

LIBDPLIBS+= \
	event	${.CURDIR}/../libevent \
	crypto	${NETBSDSRCDIR}/crypto/external/bsd/openssl/lib/libcrypto \
	ssl	${NETBSDSRCDIR}/crypto/external/bsd/openssl/lib/libssl

.include <bsd.lib.mk>
