#	$NetBSD: std.evbsh5.el,v 1.2.26.1 2006/06/21 14:51:09 yamt Exp $
#
# Options/devices that all Little Endian evbsh5s should have
#

machine evbsh5 sh5
include		"conf/std"	# MI standard options

options 	EXEC_ELF32
options 	EXEC_SCRIPT

makeoptions 	MACHINE_ARCH=sh5el
