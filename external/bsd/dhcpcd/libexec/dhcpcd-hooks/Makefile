# $NetBSD: Makefile,v 1.5 2020/04/03 15:02:11 roy Exp $
#

.include <bsd.own.mk>

.include "../../Makefile.inc"
HOOKSRC=	${DIST}/hooks
.PATH:		${HOOKSRC}

FILES=		01-test 02-dump 20-resolv.conf 30-hostname
FILES+=		50-ntp.conf

FILESDIR=	/libexec/dhcpcd-hooks

FILESBUILD_30-hostname=   YES
CLEANFILES=	30-hostname

.SUFFIXES:	.in

.in:	../../Makefile.inc
	${TOOL_SED} ${SED_SYS} $< > $@

.include <bsd.prog.mk>
