#	$NetBSD: Makefile,v 1.7 2026/07/12 20:07:45 martin Exp $
#
# Makefile for atari
#

#MD_OPTIONS=	AOUT2ELF
LANGUAGES=	en de

CPPFLAGS+=	-DSUPPORT_8_3_SOURCE_FILESYSTEM

.ifdef SMALLPROG
CHECK_ENTROPY=	no	# do not check entropy/offer adding
NOPARTMAN=	yes
NO_GPT=		yes
NO_DISKSIZE_CHECKS=	yes	# we only have disklabel, can't do large disks
NO_MBR=		yes
NO_CLONES=	yes
.endif

.include "../../Makefile.inc"
