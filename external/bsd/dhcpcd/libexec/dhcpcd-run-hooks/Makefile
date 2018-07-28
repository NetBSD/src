# $NetBSD: Makefile,v 1.1.18.2 2018/07/28 13:20:46 martin Exp $
#

.include <bsd.own.mk>

.include "../../Makefile.inc"
HOOKSRC=	${DIST}/hooks
.PATH:		${HOOKSRC}

SCRIPTS=	dhcpcd-run-hooks
SCRIPTSDIR=	/libexec

MAN=		dhcpcd-run-hooks.8

CLEANFILES=	dhcpcd-run-hooks dhcpcd-run-hooks.8

.for f in ${SCRIPTS} ${MAN}
${f}:	../../Makefile.inc ${f}.in
	${TOOL_SED} ${SED_SYS} ${HOOKSRC}/${f}.in > $@
.endfor

.include <bsd.prog.mk>
