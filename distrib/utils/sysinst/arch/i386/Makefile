#	$NetBSD: Makefile,v 1.8 2000/01/04 08:33:52 itojun Exp $
#
# Makefile for i386
#

# have to have all of them here for crunch ...

SRCS=	menu_defs.c msg_defs.c main.c install.c upgrade.c \
	txtwalk.c run.c factor.c net.c disks.c util.c geom.c \
	label.c target.c md.c mbr.c

fdisk.o md.o:	menu_defs.h msg_defs.h

CPPFLAGS+=-DINET6

.include "../../Makefile.inc"
