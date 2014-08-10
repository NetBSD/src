#	$NetBSD: Makefile,v 1.1.2.2 2014/08/10 07:00:26 tls Exp $
#
# Makefile for i386
#

MENUS_MD=	menus.md menus.mbr
MSG_MD=		msg.md.${SYSINSTLANG} msg.mbr.${SYSINSTLANG}
MD_OPTIONS=	BOOTSEL AOUT2ELF HAVE_INSTALL_IMAGE

CPPFLAGS+=	-DBOOTSEL -DWSKBD
# CPPFLAGS+=	-DNO_LBA_READS

.include "../../Makefile.inc"
