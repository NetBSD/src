#	$NetBSD: Makefile,v 1.12 2003/05/16 19:48:29 dsl Exp $
#
# Makefile for i386
#

# have to have all of them here for crunch ...

SRCS=	menu_defs.c msg_defs.c main.c install.c upgrade.c \
	txtwalk.c run.c factor.c net.c disks.c disks_lfs.c util.c geom.c \
	label.c target.c md.c mbr.c aout2elf.c

fdisk.o md.o:	menu_defs.h msg_defs.h

MENUS_MD=	menus.md menus.mbr
MSG_MD=		msg.md.${SYSINSTLANG} msg.mbr.${SYSINSTLANG}
MD_OPTIONS=	BOOTSEL

CPPFLAGS+=	-DBOOTSEL

.include "../../Makefile.inc"
