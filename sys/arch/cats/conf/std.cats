#	$NetBSD: std.cats,v 1.1.8.2 2002/01/08 00:23:49 nathanw Exp $
#
# standard NetBSD/cats options

machine	cats arm

options 	EXEC_AOUT
options 	EXEC_SCRIPT
options 	EXEC_ELF32

# To support easy transit to ../arch/arm/arm32
options 	ARM32
