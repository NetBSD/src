#	$NetBSD: Makefile,v 1.7 2005/02/19 17:00:38 dsl Exp $
#
# Makefile for amd64
#

MENUS_MD=	../i386/menus.md menus.mbr
MSG_MD=		../i386/msg.md.${SYSINSTLANG} msg.mbr.${SYSINSTLANG}
MD_OPTIONS=	BOOTSEL AOUT2ELF

CPPFLAGS+=	-DBOOTSEL -DWSKBD

.include "../../Makefile.inc"
