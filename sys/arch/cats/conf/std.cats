#	$NetBSD: std.cats,v 1.1.2.1 2002/01/10 19:40:56 thorpej Exp $
#
# standard NetBSD/cats options

machine	cats arm

options 	EXEC_AOUT
options 	EXEC_SCRIPT
options 	EXEC_ELF32

# To support easy transit to ../arch/arm/arm32
options 	ARM32
