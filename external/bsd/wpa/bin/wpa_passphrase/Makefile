# $NetBSD: Makefile,v 1.6 2023/05/09 21:05:24 christos Exp $

.include "${.CURDIR}/../Makefile.inc"

.PATH.c: ${WPA_SUPPLICANT_DISTDIR} ${.CURDIR}/../wpa_supplicant ${COMMON_PATH}

PROG=	wpa_passphrase
SRCS= \
common.c \
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
CPPFLAGS+= -DCONFIG_CRYPTO_INTERNAL
SRCS+=	tls_none.c md5-internal.c sha1-internal.c
SRCS+= sha1-pbkdf2.c sha1.c md5.c
.endif


MAN=	wpa_passphrase.8
COPTS.crypto_openssl.c+= -Wno-error=deprecated-declarations

.include <bsd.prog.mk>
