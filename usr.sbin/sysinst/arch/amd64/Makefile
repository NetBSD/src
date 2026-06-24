#	$NetBSD: Makefile,v 1.2.14.1 2026/06/24 07:14:09 jdc Exp $
#
# Makefile for amd64
#

MENUS_MD=	../i386/menus.md
MSG_MD=		../i386/msg.md.${SYSINSTLANG} msg.mbr.${SYSINSTLANG}
MD_OPTIONS=	BOOTSEL AOUT2ELF HAVE_INSTALL_IMAGE
NO_MBR=no
HAVE_DVD_IMAGE=yes

CPPFLAGS+=	-DBOOTSEL -DWSKBD

.include "../../Makefile.inc"
