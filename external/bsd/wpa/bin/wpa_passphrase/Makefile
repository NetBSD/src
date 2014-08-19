# $NetBSD: Makefile,v 1.1.12.1 2014/08/19 23:52:16 tls Exp $

.include "${.CURDIR}/../Makefile.inc"

.PATH.c: ${WPA_SUPPLICANT_DISTDIR} ${.CURDIR}/../wpa_supplicant

PROG=	wpa_passphrase
SRCS= \
md5.c \
os_unix.c \
wpa_debug.c \
wpa_passphrase.c \
wpabuf.c 

.if !defined(NO_CRYPT) && !defined(NO_OPENSSL) && !defined(RELEASE_CRUNCH)
SRCS+= crypto_openssl.c
DPADD+= ${LIBSSL} ${LIBCRYPTO} ${LIBDES}
LDADD+= -lssl -lcrypto -ldes
.else
CPPFLAGS+= -DINTERNAL_AES -DINTERNAL_MD5 -DINTERNAL_SHA1
SRCS+=	tls_none.c md5-internal.c sha1-internal.c
.endif


MAN=	wpa_passphrase.8

.include <bsd.prog.mk>
