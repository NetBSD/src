# $NetBSD: Makefile,v 1.1 2021/12/28 17:05:44 christos Exp $

USE_FORT=yes
LIBISPRIVATE=yes

.include <bsd.init.mk>

LIB=back_ldif

SLAPD= ${LDAP_DISTDIR}/servers/slapd
BACK_LDIF=${SLAPD}/back-ldif
.PATH: ${BACK_LDIF}

CPPFLAGS+=-I${SLAPD} -I${BACK_LDIF}

SRCS += \
	ldif.c version.c

.include <bsd.lib.mk>
