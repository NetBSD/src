#	$NetBSD: std.viper,v 1.4 2008/01/27 12:37:12 chris Exp $
#
# Arcom Viper standard kernel options
#

machine evbarm arm
include		"conf/std"	# MI standard options
include		"arch/arm/conf/std.arm"	# arch standard options

include "arch/evbarm/conf/files.viper"

options 	EXEC_ELF32
options 	EXEC_SCRIPT

options 	ARM32

makeoptions	LOADADDRESS="0xc0200000"
makeoptions	BOARDTYPE="viper"
makeoptions	BOARDMKFRAG="${THISARM}/conf/mk.viper"

options         ARM_INTR_IMPL="<arch/arm/xscale/pxa2x0_intr.h>"

# OS Timer    This is compatible to SA1110's OS Timer.
saost*  at pxaip? addr 0x40a00000 size 0x20
