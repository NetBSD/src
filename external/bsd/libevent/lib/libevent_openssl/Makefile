#	$NetBSD: Makefile,v 1.4 2025/07/14 10:25:51 kre Exp $

.include <bsd.own.mk>

LIB=event_openssl

SRCS= \
bufferevent_openssl.c

LIBDPLIBS+= \
	event	${.CURDIR}/../libevent \
	crypto	${NETBSDSRCDIR}/crypto/external/${EXTERNAL_OPENSSL_SUBDIR}/lib/libcrypto \
	ssl	${NETBSDSRCDIR}/crypto/external/${EXTERNAL_OPENSSL_SUBDIR}/lib/libssl

.include <bsd.lib.mk>
