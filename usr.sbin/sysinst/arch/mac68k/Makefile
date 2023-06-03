#	$NetBSD: Makefile,v 1.4 2023/06/03 21:33:25 lukem Exp $
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

COPTS.md.c=	${CC_WNO_STRINGOP_TRUNCATION}

.include "../../Makefile.inc"
