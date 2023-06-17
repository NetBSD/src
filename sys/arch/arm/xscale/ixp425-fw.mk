#	$NetBSD: ixp425-fw.mk,v 1.3 2023/06/17 11:57:49 rin Exp $

#
# For IXP425 NE support, this file must be included by the board-specific
# Makefile stub.
#

#
# See ixp425-fw.README for instructions on how to download and generate
# a suitable microcode image for IXP425 Ethernet support.
#

NPE_MICROCODE=	$S/arch/arm/xscale/IxNpeMicrocode.dat
.if exists(${MICROCODE})
CPPFLAGS+=	-DIXP425_NPE_MICROCODE=\"${NPE_MICROCODE}\"
.endif
