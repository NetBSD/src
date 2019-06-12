#	$NetBSD: Makefile,v 1.2 2019/06/12 06:20:18 martin Exp $
#
# Makefile for amd64
#

MENUS_MD=	../i386/menus.md
MSG_MD=		../i386/msg.md.${SYSINSTLANG} msg.mbr.${SYSINSTLANG}
MD_OPTIONS=	BOOTSEL AOUT2ELF HAVE_INSTALL_IMAGE
NO_MBR=no

CPPFLAGS+=	-DBOOTSEL -DWSKBD

.include "../../Makefile.inc"
