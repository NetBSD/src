# $NetBSD: Makefile,v 1.2 2019/07/26 10:34:26 roy Exp $
#

.include <bsd.own.mk>

.include "../../Makefile.inc"
.PATH:			${DIST}/hooks

.if ${MKSHARE} != "no"
FILES=			15-timezone 29-lookup-hostname

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
