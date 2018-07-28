# $NetBSD: Makefile,v 1.1.18.2 2018/07/28 13:20:45 martin Exp $
#

.include <bsd.own.mk>

.include "../../Makefile.inc"
.PATH:			${DIST}/hooks

.if ${MKSHARE} != "no"
FILES=			10-wpa_supplicant 15-timezone 29-lookup-hostname
FILES+=			50-ypbind

FILESBUILD_50-ypbind=	YES
CLEANFILES+=		50-ypbind

FILESDIR=		/usr/share/examples/dhcpcd/hooks

.SUFFIXES:		.in

.in:	../../Makefile.inc
	${TOOL_SED} ${SED_SYS} $< > $@
.endif

.include <bsd.prog.mk>
