# $NetBSD: Makefile,v 1.3.18.2 2018/07/28 13:20:46 martin Exp $
#

.include <bsd.own.mk>

.include "../../Makefile.inc"

.PATH:			${DIST}/hooks

FILES=			01-test 02-dump 20-resolv.conf 30-hostname
FILES+=			50-ntp.conf

FILESDIR=		/libexec/dhcpcd-hooks

.include <bsd.prog.mk>
