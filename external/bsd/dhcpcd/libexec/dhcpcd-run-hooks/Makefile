# $NetBSD: Makefile,v 1.1.4.2 2017/04/26 02:52:25 pgoyette Exp $
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
