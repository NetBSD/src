#	$NetBSD: Makefile,v 1.6.4.1 2005/07/24 02:25:25 snj Exp $
#
# Makefile for amd64
#

MENUS_MD=	../i386/menus.md menus.mbr
MSG_MD=		../i386/msg.md.${SYSINSTLANG} msg.mbr.${SYSINSTLANG}
MD_OPTIONS=	BOOTSEL AOUT2ELF

CPPFLAGS+=	-DBOOTSEL -DWSKBD

.include "../../Makefile.inc"
