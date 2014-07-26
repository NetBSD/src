#	$NetBSD: Makefile,v 1.1 2014/07/26 19:30:44 dholland Exp $
#
# Makefile for amd64
#

MENUS_MD=	../i386/menus.md menus.mbr
MSG_MD=		../i386/msg.md.${SYSINSTLANG} msg.mbr.${SYSINSTLANG}
MD_OPTIONS=	BOOTSEL AOUT2ELF HAVE_INSTALL_IMAGE

CPPFLAGS+=	-DBOOTSEL -DWSKBD

.include "../../Makefile.inc"
