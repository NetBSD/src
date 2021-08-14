#	$NetBSD: Makefile,v 1.6 2021/08/14 16:15:04 christos Exp $

USE_FORT?=	yes

LIBISPRIVATE=	yes

.include "../../openldap.mk"

LIB=		lutil

.PATH:		${LDAP_DISTDIR}/libraries/liblutil
CPPFLAGS+=	-I${LDAP_DISTDIR}/libraries/liblutil

SRCS=		base64.c entropy.c sasl.c signal.c hash.c passfile.c \
		md5.c passwd.c sha1.c getpass.c lockf.c utils.c uuid.c \
		sockpair.c meter.c \
		getpeereid.c detach.c

.include <bsd.lib.mk>
