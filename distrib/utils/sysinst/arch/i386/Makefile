#	$NetBSD: Makefile,v 1.10 2002/02/03 22:21:56 skrll Exp $
#
# Makefile for i386
#

# have to have all of them here for crunch ...

SRCS=	menu_defs.c msg_defs.c main.c install.c upgrade.c \
	txtwalk.c run.c factor.c net.c disks.c disks_lfs.c util.c geom.c \
	label.c target.c md.c mbr.c aout2elf.c

fdisk.o md.o:	menu_defs.h msg_defs.h

CPPFLAGS+=-DINET6

.include "../../Makefile.inc"
