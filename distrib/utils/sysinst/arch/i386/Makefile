#	$NetBSD: Makefile,v 1.3 1997/10/22 02:17:35 lukem Exp $
#
# Makefile for i386
#

# have to have all of them here for crunch ...

SRCS=	menu_defs.c msg_defs.c main.c install.c upgrade.c \
	txtwalk.c run.c factor.c net.c disks.c util.c geom.c \
	md.c fdisk.c

fdisk.o md.o:	menu_defs.h msg_defs.h

.include "../../Makefile.inc"
