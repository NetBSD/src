# $NetBSD: Makefile,v 1.3.16.2 2020/04/21 18:41:55 martin Exp $
#

.include <bsd.own.mk>

.include "../../Makefile.inc"
HOOKSRC=	${DIST}/hooks
.PATH:		${HOOKSRC}

FILES=		01-test 20-resolv.conf 30-hostname
FILES+=		50-ntp.conf

FILESDIR=	/libexec/dhcpcd-hooks

FILESBUILD_30-hostname=   YES
CLEANFILES=	30-hostname

.SUFFIXES:	.in

.in:	../../Makefile.inc
	${TOOL_SED} ${SED_SYS} $< > $@

.include <bsd.prog.mk>
