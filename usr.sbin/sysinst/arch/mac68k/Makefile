#	$NetBSD: Makefile,v 1.2.2.1 2020/10/15 19:36:51 bouyer Exp $
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
