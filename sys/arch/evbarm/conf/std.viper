#	$NetBSD: std.viper,v 1.3.46.1 2008/02/28 21:47:49 rjs Exp $
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
