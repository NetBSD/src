#	$NetBSD: Makefile,v 1.3 2020/08/10 06:54:45 rin Exp $
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

# XXX
COPTS.md.c=	-Wno-stringop-truncation

.include "../../Makefile.inc"
