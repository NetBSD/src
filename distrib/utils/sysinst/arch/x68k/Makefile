#	$NetBSD: Makefile,v 1.6 2003/05/31 08:10:02 dsl Exp $
#
# Makefile for x68k
#

# have to have all of them here for crunch ...

SRCS=	menu_defs.c msg_defs.c main.c install.c upgrade.c \
	txtwalk.c run.c factor.c net.c disks.c disks_lfs.c util.c geom.c \
	label.c target.c md.c savenewlabel.c sizemultname.c \
	bsddisklabel.c

md.o:	menu_defs.h msg_defs.h

.include "../../Makefile.inc"

# Note: we have no french messages, because x68k console does not
# support ISO-8859-1 (right half) character set.
