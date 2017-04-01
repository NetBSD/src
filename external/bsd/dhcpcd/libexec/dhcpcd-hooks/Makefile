# $NetBSD: Makefile,v 1.2 2017/04/01 19:02:58 roy Exp $
#

.include <bsd.own.mk>

.include "../../Makefile.inc"
HOOKSRC=		${DIST}/hooks
.PATH:			${HOOKSRC} ${OBJDIR}

HOOKS=			01-test 02-dump 20-resolv.conf 30-hostname
HOOKS+=			50-ntp.conf

FILES=			${HOOKS:C,^,${HOOKSRC}/,}
FILESDIR=		/libexec/dhcpcd-hooks

EGHOOKS=		10-wpa_supplicant 15-timezone 29-lookup-hostname
MKEGHOOKS=		50-ypbind
CLEANFILES+=		50-ypbind

.if ${MKSHARE} != "no"
EGFILES=		${EGHOOKS:C,^,${HOOKSRC}/,}
EGFILES+=		${MKEGHOOKS:C,^,${.OBJDIR}/,}
FILES+=			${EGFILES}
.for f in ${EGFILES}
FILESDIR_${f}=		/usr/share/examples/dhcpcd/hooks
.endfor
.endif

.SUFFIXES:		.in

all:	${HOOKS} ${EGHOOKS} ${MKEGHOOKS}

.in:	../../Makefile.inc
	${TOOL_SED} ${SED_SYS} $< > $@

.include <bsd.prog.mk>
