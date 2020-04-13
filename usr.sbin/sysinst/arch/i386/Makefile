#	$NetBSD: Makefile,v 1.1.28.1 2020/04/13 08:06:03 martin Exp $
#
# Makefile for i386
#

MENUS_MD=	menus.md
MSG_MD=		msg.md.${SYSINSTLANG} msg.mbr.${SYSINSTLANG}
MD_OPTIONS=	BOOTSEL AOUT2ELF HAVE_INSTALL_IMAGE
NO_MBR=no

CPPFLAGS+=	-DBOOTSEL -DWSKBD
# CPPFLAGS+=	-DNO_LBA_READS

.include "../../Makefile.inc"
