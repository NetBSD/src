#	$NetBSD: std.evbsh5.el,v 1.2.10.1 2005/11/10 13:56:05 skrll Exp $
#
# Options/devices that all Little Endian evbsh5s should have
#

machine evbsh5 sh5
include		"conf/std"	# MI standard options

options 	EXEC_ELF32
options 	EXEC_SCRIPT

makeoptions 	MACHINE_ARCH=sh5el
