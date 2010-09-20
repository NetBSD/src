#	$NetBSD: Makefile,v 1.12 2010/09/20 23:11:38 mrg Exp $
#
#	$eterna: Makefile,v 1.30 2010/07/11 00:34:27 mrg Exp $
#
# berkeley (netbsd) makefile.  see Makefile.boot for other systems.

# compile-time options are:
#	NO_DEBUG		/* don't include debugging support */
#	NO_USER_SUPPORT		/* don't support /~user requests */
#	NO_CGIBIN_SUPPORT	/* don't support cgi-bin requests */
#	NO_DIRINDEX_SUPPORT	/* don't support directory indexing */
#	NO_DAEMON_MODE		/* don't support daemon mode */
#	NO_DYNAMIC_CONTENT	/* don't support dynamic content updates */
#	NO_SSL_SUPPORT		/* don't support ssl (https) */
#	DO_HTPASSWD		/* support .htpasswd files */
#
# these are usually set via the "COPTS" variable, or some other method
# for setting CFLAGS relevant to your make, eg
#   % make COPTS="-DDO_HTPASSWD"

COPTS+=	-DDO_HTPASSWD
PROG=	httpd
MAN=	httpd.8
BUILDSYMLINKS+=bozohttpd.8 httpd.8
SRCS=	bozohttpd.c ssl-bozo.c auth-bozo.c cgi-bozo.c daemon-bozo.c \
	tilde-luzah-bozo.c dir-index-bozo.c content-bozo.c
SRCS+=	main.c

LDADD=	-lcrypt
DPADD=	${LIBCRYPT}

WARNS?=	4

.include <bsd.own.mk>

.if ${MKCRYPTO} != "no"

LDADD+=	-lssl -lcrypto
DPADD+=	${LIBSSL} ${LIBCRYPTO}

.else

COPTS+=	-DNO_SSL_SUPPORT

.endif

.include <bsd.prog.mk>
