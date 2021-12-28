# $NetBSD: Makefile,v 1.1 2021/12/28 17:05:44 christos Exp $

USE_FORT=yes
LIBISPRIVATE=yes

.include <bsd.init.mk>

LIB=back_monitor

SLAPD= ${LDAP_DISTDIR}/servers/slapd
BACK_MONITOR=${SLAPD}/back-monitor
.PATH: ${BACK_MONITOR} 

CPPFLAGS+=-I${SLAPD} -I${BACK_MONITOR}

SRCS += \
	backend.c bind.c cache.c compare.c conn.c database.c entry.c init.c \
	listener.c log.c modify.c operation.c operational.c overlay.c rww.c \
	search.c sent.c thread.c time.c version.c


.include <bsd.lib.mk>
