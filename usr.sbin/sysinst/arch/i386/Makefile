#	$NetBSD: Makefile,v 1.2.14.1 2026/06/24 07:14:09 jdc Exp $
#
# Makefile for i386
#

MENUS_MD=	menus.md
MSG_MD=		msg.md.${SYSINSTLANG} msg.mbr.${SYSINSTLANG}
MD_OPTIONS=	BOOTSEL AOUT2ELF HAVE_INSTALL_IMAGE
NO_MBR=no
HAVE_DVD_IMAGE=yes

CPPFLAGS+=	-DBOOTSEL -DWSKBD
# CPPFLAGS+=	-DNO_LBA_READS

.include "../../Makefile.inc"
