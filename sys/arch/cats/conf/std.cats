#	$NetBSD: std.cats,v 1.4 2003/04/26 17:35:57 chris Exp $
#
# standard NetBSD/cats options

machine	cats arm

options 	EXEC_AOUT
options 	EXEC_SCRIPT
options 	EXEC_ELF32

# To support easy transit to ../arch/arm/arm32
options 	ARM32

# New pmap options are standard for cats
options 	ARM32_PMAP_NEW
#options 	ARM32_NEW_VM_LAYOUT	# Not yet supported

options 	ARM_INTR_IMPL="<arm/footbridge/footbridge_intr.h>"
