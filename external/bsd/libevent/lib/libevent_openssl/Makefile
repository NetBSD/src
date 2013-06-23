#	$NetBSD: Makefile,v 1.1.4.2 2013/06/23 06:28:18 tls Exp $

.include <bsd.own.mk>

LIB=event_openssl

SRCS= \
bufferevent_openssl.c

LIBDPLIBS+=     crypto  ${NETBSDSRCDIR}/crypto/external/bsd/openssl/lib/libcrypto \
	crypt  ${NETBSDSRCDIR}/lib/libcrypt

.include <bsd.lib.mk>
