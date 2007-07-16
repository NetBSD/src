# $NetBSD: Makefile,v 1.2 2007/07/16 19:33:05 joerg Exp $

PROG=		audit-packages
SRCS=		audit-packages.c
MAN=		audit-packages.1 audit-packages.conf.5
SCRIPTS=	download-vulnerability-list.sh
MLINKS=		audit-packages.1 download-vulnerability-list.1
CPPFLAGS+=	-DNETBSD

.include <bsd.init.mk>

.PATH:	${DIST}/audit-packages

SUBST_PATTERNS=	\
	-e 's|@pkgdbdir@|/var/db/pkg|' \
	-e 's|@sysconfdir@|/etc|' \
	-e 's|@exec_prefix@|/usr|' \
	-e 's|@prefix@|/usr/pkg|' \
	-e 's|@ftp@|/usr/bin/ftp|' \
	-e 's|@gzcat@|/usr/bin/gzcat|'

CLEANFILES+= audit-packages.1 audit-packages.conf audit-packages.conf.5 \
	download-vulnerability-list.1 download-vulnerability-list.sh

audit-packages.1: ${DIST}/audit-packages/audit-packages.1.in
	${TOOL_SED} ${SUBST_PATTERNS} ${DIST}/audit-packages/audit-packages.1.in > ${.TARGET}
audit-packages.conf: ${DIST}/audit-packages/audit-packages.conf.in
	${TOOL_SED} ${SUBST_PATTERNS} ${DIST}/audit-packages/audit-packages.conf.in > ${.TARGET}
audit-packages.conf.5: ${DIST}/audit-packages/audit-packages.conf.5.in
	${TOOL_SED} ${SUBST_PATTERNS} ${DIST}/audit-packages/audit-packages.conf.5.in > ${.TARGET}
download-vulnerability-list.1: ${DIST}/audit-packages/download-vulnerability-list.1.in
	${TOOL_SED} ${SUBST_PATTERNS} ${DIST}/audit-packages/download-vulnerability-list.1.in > ${.TARGET}
download-vulnerability-list.sh: ${DIST}/audit-packages/download-vulnerability-list.sh.in
	${TOOL_SED} ${SUBST_PATTERNS} ${DIST}/audit-packages/download-vulnerability-list.sh.in > ${.TARGET}

.include <bsd.prog.mk>
