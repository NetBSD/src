#	$NetBSD: Makefile,v 1.3 2002/06/21 08:20:09 itojun Exp $
#
# Makefile for macppc
#

# have to have all of them here for crunch ...

SRCS=	menu_defs.c msg_defs.c main.c install.c upgrade.c \
	txtwalk.c run.c factor.c net.c disks.c disks_lfs.c util.c geom.c \
	label.c target.c md.c

md.o:	menu_defs.h msg_defs.h

CPPFLAGS+=-DINET6

.include "../../Makefile.inc"
