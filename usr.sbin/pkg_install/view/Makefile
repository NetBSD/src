# $NetBSD: Makefile,v 1.3 2007/07/16 19:57:46 joerg Exp $

SCRIPTS= linkfarm.sh pkg_view.sh
MAN= linkfarm.1 pkg_view.1

.include <bsd.init.mk>

.PATH:	${DIST}/view

SUBST_PATTERNS= \
	-e 's|@prefix@|/usr|' \
	-e 's|@exec_prefix@|/usr|' \
	-e 's|@sbindir@|/usr/sbin|' \
	-e 's|@CHMOD@|/bin/chmod|' \
	-e 's|@CMP@|/usr/bin/cmp|' \
	-e 's|@CP@|/bin/cp|' \
	-e 's|@ENV@|/usr/bin/env|' \
	-e 's|@FIND@|/usr/bin/find|' \
	-e 's|@GREP@|/usr/bin/grep|' \
	-e 's|@LN@|/bin/ln|' \
	-e 's|@MKDIR@|/bin/mkdir|' \
	-e 's|@pax@|/bin/pax|' \
	-e 's|@RM@|/bin/rm|' \
	-e 's|@RMDIR@|/bin/rmdir|' \
	-e 's|@SED@|/usr/bin/sed|' \
	-e 's|@SORT@|/usr/bin/sort|' \
	-e 's|@TOUCH@|/usr/bin/touch|'

CLEANFILES+=	linkfarm.sh pkg_view.sh

linkfarm.sh: ${DIST}/view/linkfarm.sh.in
	${TOOL_SED} ${SUBST_PATTERNS} ${DIST}/view/linkfarm.sh.in > ${.TARGET}
pkg_view.sh: ${DIST}/view/pkg_view.sh.in
	${TOOL_SED} ${SUBST_PATTERNS} ${DIST}/view/pkg_view.sh.in > ${.TARGET}

.include <bsd.prog.mk>
