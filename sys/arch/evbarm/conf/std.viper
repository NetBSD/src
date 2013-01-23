#	$NetBSD: std.viper,v 1.6.30.1 2013/01/23 00:05:45 yamt Exp $
#
# Arcom Viper standard kernel options
#

machine evbarm arm
include 	"arch/evbarm/conf/std.evbarm"

include		"arch/evbarm/conf/files.viper"

options 	KERNEL_BASE_EXT=0xc0000000
makeoptions	LOADADDRESS="0xc0200000"
makeoptions	BOARDTYPE="viper"
makeoptions	BOARDMKFRAG="${THISARM}/conf/mk.viper"

options         ARM_INTR_IMPL="<arch/arm/xscale/pxa2x0_intr.h>"

# OS Timer    This is compatible to SA1110's OS Timer.
saost*  at pxaip? addr 0x40a00000 size 0x20
