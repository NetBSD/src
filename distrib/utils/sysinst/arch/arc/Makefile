#	$NetBSD: Makefile,v 1.2 2002/10/17 02:05:37 lukem Exp $
#
# Makefile for arc
#

# have to have all of them here for crunch ...

SRCS=	menu_defs.c msg_defs.c main.c install.c upgrade.c \
	txtwalk.c run.c factor.c net.c disks.c disks_lfs.c util.c geom.c \
	label.c target.c md.c mbr.c

fdisk.o md.o:	menu_defs.h msg_defs.h

.include "../../Makefile.inc"
