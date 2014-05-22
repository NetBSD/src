#	$NetBSD: Makefile,v 1.2.4.2 2014/05/22 15:48:11 yamt Exp $

.include <bsd.own.mk>

LIB=event_openssl

SRCS= \
bufferevent_openssl.c

LIBDPLIBS+= \
	event	${.CURDIR}/../libevent \
	crypto	${NETBSDSRCDIR}/crypto/external/bsd/openssl/lib/libcrypto \
	ssl	${NETBSDSRCDIR}/crypto/external/bsd/openssl/lib/libssl

.include <bsd.lib.mk>
