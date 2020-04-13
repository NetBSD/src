#	$NetBSD: Makefile,v 1.1.28.1 2020/04/13 08:06:03 martin Exp $
#
# Makefile for mac68k
#

MD_OPTIONS=	AOUT2ELF
NODISKLABEL=	yes
LANGUAGES=	en pl de

.ifdef SMALLPROG
NOPARTMAN=	yes
NO_GPT=		yes
NO_MBR=		yes
.endif

.include "../../Makefile.inc"
