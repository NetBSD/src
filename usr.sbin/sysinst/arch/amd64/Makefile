#	$NetBSD: Makefile,v 1.1.28.1 2020/04/13 08:06:00 martin Exp $
#
# Makefile for amd64
#

MENUS_MD=	../i386/menus.md
MSG_MD=		../i386/msg.md.${SYSINSTLANG} msg.mbr.${SYSINSTLANG}
MD_OPTIONS=	BOOTSEL AOUT2ELF HAVE_INSTALL_IMAGE
NO_MBR=no

CPPFLAGS+=	-DBOOTSEL -DWSKBD

.include "../../Makefile.inc"
