# $NetBSD: Makefile,v 1.1.4.2 2017/04/26 02:52:24 pgoyette Exp $
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
