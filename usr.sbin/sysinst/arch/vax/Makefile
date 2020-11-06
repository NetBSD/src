#	$NetBSD: Makefile,v 1.2 2020/11/06 19:47:31 martin Exp $
#
# Makefile for vax
#

MD_OPTIONS=	AOUT2ELF

.ifdef SMALLPROG
CHECK_ENTROPY=	no	# do not check entropy/offer adding
NOPARTMAN=	yes
NO_GPT=		yes
NO_MBR=		yes
NO_CLONES=	yes
.endif

.include "../../Makefile.inc"
