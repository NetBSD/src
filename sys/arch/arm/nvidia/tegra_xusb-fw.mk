#	$NetBSD: tegra_xusb-fw.mk,v 1.1 2016/09/26 20:05:03 jakllsch Exp $

.if defined(TEGRA124_XUSB_BIN_STATIC)
MD_OBJS+=	tegra124_xusb_bin.o
CPPFLAGS+=	-DTEGRA124_XUSB_BIN_STATIC

tegra124_xusb_bin.o:	$S/arch/arm/nvidia/tegra124_xusb.bin
	-rm -f ${.OBJDIR}/tegra124_xusb.bin
	-ln -s $S/arch/arm/nvidia/tegra124_xusb.bin ${.OBJDIR}
	${OBJCOPY} -I binary -O default -B arm tegra124_xusb.bin \
	    --rename-section .data=.rodata ${.TARGET}
.endif
