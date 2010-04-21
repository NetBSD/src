# $NetBSD: Makefile,v 1.1.8.1.2.1 2010/04/21 05:23:10 matt Exp $

PROG= pkg_admin
SRCS= audit.c check.c main.c

SCRIPTS=	audit-packages.sh download-vulnerability-list.sh
CLEANFILES=	audit-packages.sh download-vulnerability-list.sh

audit-packages.sh: audit-packages.sh.in
	sed -e 's|@PKG_ADMIN@|/usr/sbin/pkg_admin|' \
	    ${DIST}/admin/audit-packages.sh.in > $@

download-vulnerability-list.sh: download-vulnerability-list.sh.in
	sed -e 's|@PKG_ADMIN@|/usr/sbin/pkg_admin|' \
	    ${DIST}/admin/download-vulnerability-list.sh.in > $@

LINK_PKGVUL=	yes

.include <bsd.init.mk>

.PATH:	${DIST}/admin
.include <bsd.prog.mk>
