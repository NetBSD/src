#	$NetBSD: Makefile,v 1.15 2003/06/12 11:20:07 dsl Exp $
#
# Makefile for i386
#

# have to have all of them here for crunch ...

SRCS=	menu_defs.c msg_defs.c main.c install.c upgrade.c \
	txtwalk.c run.c factor.c net.c disks.c disks_lfs.c util.c geom.c \
	label.c target.c md.c mbr.c aout2elf.c sizemultname.c \
	bsddisklabel.c savenewlabel.c

MENUS_MD=	menus.md menus.mbr
MSG_MD=		msg.md.${SYSINSTLANG} msg.mbr.${SYSINSTLANG}
MD_OPTIONS=	BOOTSEL AOUT2ELF

CPPFLAGS+=	-DBOOTSEL
# CPPFLAGS+=	-DNO_LBA_READS

.include "../../Makefile.inc"
