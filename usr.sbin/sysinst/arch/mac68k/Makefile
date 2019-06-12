#	$NetBSD: Makefile,v 1.2 2019/06/12 06:20:21 martin Exp $
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
