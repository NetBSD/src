#	$NetBSD: std.cats,v 1.2 2001/11/20 12:56:23 lukem Exp $
#
# standard NetBSD/cats options

machine	cats arm

options 	EXEC_AOUT
options 	EXEC_SCRIPT
options 	EXEC_ELF32

# To support easy transit to ../arch/arm/arm32
options 	ARM32
