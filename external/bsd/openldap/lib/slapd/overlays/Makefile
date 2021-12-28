# $NetBSD: Makefile,v 1.1 2021/12/28 17:05:44 christos Exp $

USE_FORT=yes
LIBISPRIVATE=yes

.include <bsd.init.mk>

LIB=overlays

SLAPD= ${LDAP_DISTDIR}/servers/slapd
OVERLAYS=${SLAPD}/overlays
.PATH: ${OVERLAYS}

CPPFLAGS+=-I${SLAPD} -I${OVERLAYS} -DSLAPD_IMPORT -DSLAPD_OVER_SYNCPROV

SRCS += \
	overlays.c statover.c syncprov.c

.include <bsd.lib.mk>
