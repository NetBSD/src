# $NetBSD: Makefile,v 1.1 2017/03/31 21:02:09 roy Exp $
#

.include <bsd.own.mk>

.include "../../Makefile.inc"
HOOKSRC=		${DIST}/hooks
.PATH:			${HOOKSRC}

HOOKS=			01-test 02-dump 20-resolv.conf 30-hostname
HOOKS+=			50-ntp.conf

FILES=			${HOOKS:C,^,${HOOKSRC}/,}
FILESDIR=		/libexec/dhcpcd-hooks

EGHOOKS=		10-wpa_supplicant 15-timezone 29-lookup-hostname
EGHOOKS+=		50-ypbind

.if ${MKSHARE} != "no"
EGFILES=		${EGHOOKS:C,^,${HOOKSRC}/,}
FILES+=			${EGFILES}
.for f in ${EGFILES}
FILESDIR_${f}=		/usr/share/examples/dhcpcd/hooks
.endfor
.endif

.include <bsd.prog.mk>
