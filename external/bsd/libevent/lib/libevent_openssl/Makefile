#	$NetBSD: Makefile,v 1.3 2018/02/04 03:19:52 christos Exp $

.include <bsd.own.mk>

LIB=event_openssl

SRCS= \
bufferevent_openssl.c

LIBDPLIBS+= \
	event	${.CURDIR}/../libevent \
	crypto	${NETBSDSRCDIR}/crypto/external/bsd/${EXTERNAL_OPENSSL_SUBDIR}/lib/libcrypto \
	ssl	${NETBSDSRCDIR}/crypto/external/bsd/${EXTERNAL_OPENSSL_SUBDIR}/lib/libssl

.include <bsd.lib.mk>
