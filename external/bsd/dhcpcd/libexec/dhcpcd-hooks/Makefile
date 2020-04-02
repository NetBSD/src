# $NetBSD: Makefile,v 1.4 2020/04/02 14:05:28 roy Exp $
#

.include <bsd.own.mk>

.include "../../Makefile.inc"
HOOKSRC=	${DIST}/hooks
.PATH:		${HOOKSRC}

FILES=		01-test 02-dump 20-resolv.conf 30-hostname
FILES+=		50-ntp.conf

FILESBUILD_30-hostname=   YES

FILESDIR=	/libexec/dhcpcd-hooks

CLEANFILES=	30-hostname

.for f in ${FILES}
${f}:	../../Makefile.inc ${f}.in
	${TOOL_SED} ${SED_SYS} ${HOOKSRC}/${f}.in > $@
.endfor

.include <bsd.prog.mk>
