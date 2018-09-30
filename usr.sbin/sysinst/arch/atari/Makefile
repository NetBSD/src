#	$NetBSD: Makefile,v 1.2.14.1 2018/09/30 01:46:01 pgoyette Exp $
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
