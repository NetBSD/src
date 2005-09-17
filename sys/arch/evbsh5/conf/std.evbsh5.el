#	$NetBSD: std.evbsh5.el,v 1.3 2005/09/17 09:44:06 yamt Exp $
#
# Options/devices that all Little Endian evbsh5s should have
#

machine evbsh5 sh5
include		"conf/std"	# MI standard options

options 	EXEC_ELF32
options 	EXEC_SCRIPT

makeoptions 	MACHINE_ARCH=sh5el
