#	$NetBSD: Makefile,v 1.7 1999/03/31 00:44:49 fvdl Exp $
#
# Makefile for i386
#

# have to have all of them here for crunch ...

SRCS=	menu_defs.c msg_defs.c main.c install.c upgrade.c \
	txtwalk.c run.c factor.c net.c disks.c util.c geom.c \
	label.c target.c md.c mbr.c

fdisk.o md.o:	menu_defs.h msg_defs.h

.include "../../Makefile.inc"
