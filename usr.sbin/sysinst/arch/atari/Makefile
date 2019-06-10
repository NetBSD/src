#	$NetBSD: Makefile,v 1.2.16.1 2019/06/10 22:10:38 christos Exp $
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
