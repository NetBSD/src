#	$NetBSD: std.evbsh5.el,v 1.2.4.2 2002/09/06 08:34:33 jdolecek Exp $
#
# Options/devices that all Little Endian evbsh5s should have
#

machine evbsh5 sh5

options 	EXEC_ELF32
options 	EXEC_SCRIPT

makeoptions 	MACHINE_ARCH=sh5el
