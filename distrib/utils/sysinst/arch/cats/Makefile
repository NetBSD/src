#	$NetBSD: Makefile,v 1.4 2003/05/30 22:17:03 dsl Exp $
#
# Makefile for cats
#

# have to have all of them here for crunch ...

SRCS=	menu_defs.c msg_defs.c main.c install.c upgrade.c \
	txtwalk.c run.c factor.c net.c disks.c disks_lfs.c util.c  geom.c \
	label.c target.c md.c savenewlabel.c sizemultname.c \
	bsddisklabel.c

SRCS+=	aout2elf.c

md.o:	menu_defs.h msg_defs.h

.include "../../Makefile.inc"
