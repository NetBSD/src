#	$NetBSD: Makefile,v 1.4 2019/06/12 06:20:18 martin Exp $
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
