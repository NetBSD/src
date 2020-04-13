#	$NetBSD: Makefile,v 1.2.16.2 2020/04/13 08:06:01 martin Exp $
#
# Makefile for atari
#

#MD_OPTIONS=	AOUT2ELF
LANGUAGES=	en de

CPPFLAGS+=	-DSUPPORT_8_3_SOURCE_FILESYSTEM

.ifdef SMALLPROG
NOPARTMAN=	yes
NO_GPT=		yes
.endif

.include "../../Makefile.inc"
