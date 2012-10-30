#	$NetBSD: std.cats,v 1.8.44.1 2012/10/30 17:19:15 yamt Exp $
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

#options 	ARM32_NEW_VM_LAYOUT	# Not yet supported

options 	ARM_INTR_IMPL="<arm/footbridge/footbridge_intr.h>"
