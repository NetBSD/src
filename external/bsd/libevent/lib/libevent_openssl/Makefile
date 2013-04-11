#	$NetBSD: Makefile,v 1.1 2013/04/11 16:59:17 christos Exp $

.include <bsd.own.mk>

LIB=event_openssl

SRCS= \
bufferevent_openssl.c

LIBDPLIBS+=     crypto  ${NETBSDSRCDIR}/crypto/external/bsd/openssl/lib/libcrypto \
	crypt  ${NETBSDSRCDIR}/lib/libcrypt

.include <bsd.lib.mk>
