#	$NetBSD: std.cats,v 1.1 2001/05/22 20:59:27 chris Exp $
#
# standard NetBSD/cats options

machine	cats arm

options 	EXEC_AOUT
options 	EXEC_SCRIPT
options		EXEC_ELF32

# To support easy transit to ../arch/arm/arm32
options		ARM32
