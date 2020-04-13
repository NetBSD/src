# $NetBSD: Makefile,v 1.1.16.1 2020/04/13 07:46:00 martin Exp $
#

.include <bsd.own.mk>

.include "../../Makefile.inc"
.PATH:			${DIST}/hooks

.if ${MKSHARE} != "no"
FILES=			10-wpa_supplicant 15-timezone 29-lookup-hostname

.if ${MKYP} != "no"
FILESBUILD_50-ypbind=	YES
FILES+=			50-ypbind
CLEANFILES+=		50-ypbind
.endif

FILESDIR=		/usr/share/examples/dhcpcd/hooks

.SUFFIXES:		.in

.in:	../../Makefile.inc
	${TOOL_SED} ${SED_SYS} $< > $@
.endif

.include <bsd.prog.mk>
