#	$NetBSD: std.evbsh5.el,v 1.2.2.2 2002/07/16 00:41:02 gehenna Exp $
#
# Options/devices that all Little Endian evbsh5s should have
#

machine evbsh5 sh5

options 	EXEC_ELF32
options 	EXEC_SCRIPT

makeoptions 	MACHINE_ARCH=sh5el
