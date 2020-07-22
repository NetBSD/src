# $NetBSD: Makefile,v 1.10 2020/07/22 19:00:20 roy Exp $
#

.include <bsd.own.mk>
.include "../../Makefile.inc"

SCRIPTS=	resolvconf
SCRIPTSDIR=	/sbin

MAN=		resolvconf.conf.5 resolvconf.8

CLEANFILES=	${SCRIPTS} ${MAN}

.for f in ${SCRIPTS} ${MAN}
${f}:	Makefile ${f}.in
	${TOOL_SED} ${RESOLVCONF_SED} ${DIST}/${f}.in >$@
.endfor

.include <bsd.prog.mk>
