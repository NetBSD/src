#	$NetBSD: Makefile,v 1.4 2000/02/28 05:17:50 jun Exp $
#
# Makefile for i386
#

# have to have all of them here for crunch ...

SRCS=	menu_defs.c msg_defs.c main.c install.c upgrade.c \
	txtwalk.c run.c factor.c net.c disks.c util.c geom.c \
	label.c target.c md.c mbr.c mbr_buf.s

fdisk.o md.o:	menu_defs.h msg_defs.h

CPPFLAGS+=-DINET6

.include "../../Makefile.inc"
