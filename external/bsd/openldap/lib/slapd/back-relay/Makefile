# $NetBSD: Makefile,v 1.1 2021/12/28 17:05:44 christos Exp $

USE_FORT=yes
LIBISPRIVATE=yes

.include <bsd.init.mk>

LIB=back_relay

SLAPD= ${LDAP_DISTDIR}/servers/slapd
BACK_RELAY=${SLAPD}/back-relay
.PATH: ${BACK_RELAY} 

CPPFLAGS+=-I${SLAPD} -I${BACK_RELAY}

SRCS += \
	init.c op.c version.c


.include <bsd.lib.mk>
