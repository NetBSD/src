#	$NetBSD: Makefile,v 1.2 2022/06/11 16:41:55 tsutsui Exp $
#
# Makefile for newsmips
#

.ifdef SMALLPROG
CHECK_ENTROPY=	no	# do not check entropy/offer adding
NOPARTMAN=	yes
NO_GPT=		yes
NO_MBR=		yes
NO_CLONES=	yes
.endif

.include "../../Makefile.inc"
