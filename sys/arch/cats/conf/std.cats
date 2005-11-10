#	$NetBSD: std.cats,v 1.5.2.1 2005/11/10 13:55:47 skrll Exp $
#
# standard NetBSD/cats options

machine	cats arm
include		"conf/std"	# MI standard options

options 	EXEC_AOUT
options 	EXEC_SCRIPT
options 	EXEC_ELF32

# To support easy transit to ../arch/arm/arm32
options 	ARM32

#options 	ARM32_NEW_VM_LAYOUT	# Not yet supported

options 	ARM_INTR_IMPL="<arm/footbridge/footbridge_intr.h>"
