#	$NetBSD: Makefile,v 1.2 2003/01/11 19:44:10 christos Exp $
#
# Makefile for hp300
#

# have to have all of them here for crunch ...

SRCS=	menu_defs.c msg_defs.c main.c install.c upgrade.c \
	txtwalk.c run.c factor.c net.c disks.c disks_lfs.c util.c geom.c \
	label.c target.c md.c savenewlabel.c sizemultname.c

fdisk.o md.o:	menu_defs.h msg_defs.h

.include "../../Makefile.inc"
