#	$NetBSD: ixp425-fw.mk,v 1.1 2006/12/10 10:01:49 scw Exp $

#
# For IXP425 NE support, this file must be included by the board-specific
# Makefile stub.
#

#
# See ixp425-fw.README for instructions on how to download and generate
# a suitable microcode image for IXP425 Ethernet support.
#

.if exists($S/arch/arm/xscale/IxNpeMicrocode.dat)
MD_OBJS+=	ixp425_fw.o
CPPFLAGS+=	-DIXP425_NPE_MICROCODE

ixp425_fw.o:	$S/arch/arm/xscale/IxNpeMicrocode.dat
	-rm -f ${.OBJDIR}/IxNpeMicrocode.dat
	-ln -s $S/arch/arm/xscale/IxNpeMicrocode.dat ${.OBJDIR}
	${LD} -b binary -d -warn-common -r -d -o ${.TARGET} IxNpeMicrocode.dat
.endif
