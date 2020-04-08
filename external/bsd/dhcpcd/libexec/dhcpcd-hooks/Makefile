# $NetBSD: Makefile,v 1.3.16.1 2020/04/08 14:04:03 martin Exp $
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
