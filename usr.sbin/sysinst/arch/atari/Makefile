#	$NetBSD: Makefile,v 1.3 2018/09/20 12:28:33 rin Exp $
#
# Makefile for atari
#

#MD_OPTIONS=	AOUT2ELF
LANGUAGES=	en de

CPPFLAGS+=	-DSUPPORT_8_3_SOURCE_FILESYSTEM

.ifdef SMALLPROG
NOPARTMAN=	yes
.endif

.include "../../Makefile.inc"
