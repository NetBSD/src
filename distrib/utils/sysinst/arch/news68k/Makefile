#	$NetBSD: Makefile,v 1.3 2002/03/16 17:26:55 tsutsui Exp $
#
# Makefile for news68k
#

# have to have all of them here for crunch ...

SRCS=	menu_defs.c msg_defs.c main.c install.c upgrade.c \
	txtwalk.c run.c factor.c net.c disks.c disks_lfs.c util.c geom.c \
	label.c target.c md.c
SRCS+=	aout2elf.c

md.o:	menu_defs.h msg_defs.h

.include "../../Makefile.inc"
