#	$NetBSD: Makefile,v 1.4 2003/05/30 22:17:05 dsl Exp $
#
# Makefile for bebox
#

# have to have all of them here for crunch ...

SRCS=	menu_defs.c msg_defs.c main.c install.c upgrade.c \
	txtwalk.c run.c factor.c net.c disks.c disks_lfs.c util.c geom.c \
	label.c target.c md.c mbr.c sizemultname.c \
	bsddisklabel.c savenewlabel.c

fdisk.o md.o:	menu_defs.h msg_defs.h

MENUS_MD=       menus.md.${SYSINSTLANG} menus.mbr
MSG_MD=         msg.md.${SYSINSTLANG} msg.mbr.${SYSINSTLANG}

.include "../../Makefile.inc"
