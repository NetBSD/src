#	$NetBSD: Makefile,v 1.1 1998/11/19 08:58:06 sakamoto Exp $
#
# Makefile for bebox
#

# have to have all of them here for crunch ...

SRCS=	menu_defs.c msg_defs.c main.c install.c upgrade.c \
	txtwalk.c run.c factor.c net.c disks.c util.c geom.c \
	label.c target.c md.c fdisk.c mbr.c

fdisk.o md.o:	menu_defs.h msg_defs.h

.include "../../Makefile.inc"
