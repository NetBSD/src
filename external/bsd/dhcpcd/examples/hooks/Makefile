# $NetBSD: Makefile,v 1.1.2.2 2017/04/02 20:18:38 roy Exp $
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
