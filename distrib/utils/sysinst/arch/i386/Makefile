#	$NetBSD: Makefile,v 1.2.2.2 1997/11/02 20:46:37 mellon Exp $
#
# Makefile for i386
#

# have to have all of them here for crunch ...

SRCS=	menu_defs.c msg_defs.c main.c install.c upgrade.c \
	txtwalk.c run.c factor.c net.c disks.c util.c geom.c \
	target.c md.c fdisk.c

fdisk.o md.o:	menu_defs.h msg_defs.h

.include "../../Makefile.inc"
