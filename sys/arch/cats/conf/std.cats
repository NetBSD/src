#	$NetBSD: std.cats,v 1.1.8.3 2003/01/03 16:41:14 thorpej Exp $
#
# standard NetBSD/cats options

machine	cats arm

options 	EXEC_AOUT
options 	EXEC_SCRIPT
options 	EXEC_ELF32

# To support easy transit to ../arch/arm/arm32
options 	ARM32

options 	ARM_INTR_IMPL="<arm/footbridge/footbridge_intr.h>"
