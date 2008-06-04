#	$NetBSD: Makefile,v 1.2.2.2 2008/06/04 02:04:30 yamt Exp $

USE_FORT?=	yes

LIBISPRIVATE=	yes

.include "../../openldap.mk"

LIB=		lutil

.PATH:		${LDAP_DISTDIR}/libraries/liblutil
CPPFLAGS+=	-I${LDAP_DISTDIR}/libraries/liblutil

SRCS=		base64.c csn.c entropy.c sasl.c signal.c hash.c passfile.c \
		md5.c passwd.c sha1.c getpass.c lockf.c utils.c uuid.c \
		sockpair.c avl.c tavl.c ldif.c fetch.c \
		getpeereid.c detach.c

.include <bsd.lib.mk>
