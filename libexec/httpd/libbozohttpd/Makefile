#	$eterna: Makefile,v 1.1 2010/05/10 02:24:31 mrg Exp $

.PATH: $(.CURDIR)/..

# build bozohttpd library
LIB=	bozohttpd
COPTS+=	-I$(.CURDIR)/..

COPTS+=	-DDO_HTPASSWD
CPPFLAGS+= -DDO_HTPASSWD
SRCS=	bozohttpd.c ssl-bozo.c auth-bozo.c cgi-bozo.c daemon-bozo.c
SRCS+=	tilde-luzah-bozo.c dir-index-bozo.c content-bozo.c
SRCS+=	lua-bozo.c

LDADD=	-lcrypt
DPADD=	${LIBCRYPT}

MAN=	libbozohttpd.3
WARNS=	4

INCS=	bozohttpd.h
INCSDIR=	/usr/include

LDADD+=	-lssl -lcrypto
DPADD+=	${LIBSSL} ${LIBCRYPTO}

COPTS.ssl-bozo.c+= -Wno-error=deprecated-declarations

.include <bsd.lib.mk>
