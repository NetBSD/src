#	$NetBSD: Makefile,v 1.15.4.1 2005/07/24 02:25:25 snj Exp $
#
# Makefile for i386
#

MENUS_MD=	menus.md menus.mbr
MSG_MD=		msg.md.${SYSINSTLANG} msg.mbr.${SYSINSTLANG}
MD_OPTIONS=	BOOTSEL AOUT2ELF

CPPFLAGS+=	-DBOOTSEL -DWSKBD
# CPPFLAGS+=	-DNO_LBA_READS

.include "../../Makefile.inc"
