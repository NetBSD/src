#	$NetBSD: Makefile,v 1.16 2004/06/12 19:18:24 dsl Exp $
#
# Makefile for i386
#

# have to have all of them here for crunch ...

SRCS=	menu_defs.c msg_defs.c main.c install.c upgrade.c \
	txtwalk.c run.c factor.c net.c disks.c disks_lfs.c util.c geom.c \
	label.c target.c md.c mbr.c aout2elf.c sizemultname.c \
	bsddisklabel.c savenewlabel.c \
	wskbd.c

MENUS_MD=	menus.md menus.mbr
MSG_MD=		msg.md.${SYSINSTLANG} msg.mbr.${SYSINSTLANG}
MD_OPTIONS=	BOOTSEL AOUT2ELF

CPPFLAGS+=	-DBOOTSEL -DWSKBD
# CPPFLAGS+=	-DNO_LBA_READS

.include "../../Makefile.inc"
