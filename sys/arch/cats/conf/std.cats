#	$NetBSD: std.cats,v 1.10 2018/01/17 20:30:16 skrll Exp $
#
# standard NetBSD/cats options

machine	cats arm
include		"conf/std"	# MI standard options
include		"arch/arm/conf/std.arm"	# arch standard options

options 	EXEC_AOUT
options 	EXEC_SCRIPT
options 	EXEC_ELF32

# To support easy transit to ../arch/arm/arm32
options 	ARM32
options 	_ARM32_NEED_BUS_DMA_BOUNCE

options 	ARM_INTR_IMPL="<arm/footbridge/footbridge_intr.h>"
