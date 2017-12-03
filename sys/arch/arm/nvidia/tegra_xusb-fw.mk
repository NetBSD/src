#	$NetBSD: tegra_xusb-fw.mk,v 1.2.2.2 2017/12/03 11:35:54 jdolecek Exp $

.if defined(TEGRA124_XUSB_BIN_STATIC)
MD_OBJS+=	tegra124_xusb_bin.o
CPPFLAGS+=	-DTEGRA124_XUSB_BIN_STATIC

tegra124_xusb_bin.o:	$S/arch/arm/nvidia/tegra124_xusb.bin
	-rm -f ${.OBJDIR}/tegra124_xusb.bin
	-ln -s $S/arch/arm/nvidia/tegra124_xusb.bin ${.OBJDIR}
	${OBJCOPY} -I binary -O default -B arm tegra124_xusb.bin \
	    --rename-section .data=.rodata ${.TARGET}
.endif

.if defined(TEGRA210_XUSB_BIN_STATIC)
MD_OBJS+=	tegra210_xusb_bin.o
CPPFLAGS+=	-DTEGRA210_XUSB_BIN_STATIC

tegra210_xusb_bin.o:	$S/arch/arm/nvidia/tegra210_xusb.bin
	-rm -f ${.OBJDIR}/tegra210_xusb.bin
	-ln -s $S/arch/arm/nvidia/tegra210_xusb.bin ${.OBJDIR}
	${OBJCOPY} -I binary -O default -B arm tegra210_xusb.bin \
	    --rename-section .data=.rodata ${.TARGET}
.endif
