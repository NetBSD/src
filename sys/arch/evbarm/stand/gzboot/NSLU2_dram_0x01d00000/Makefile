#	$NetBSD: Makefile,v 1.1 2020/02/12 06:57:35 thorpej Exp $

S=	${.CURDIR}/../../../../..

#
# This image is for the stock RedBoot configuration.  The self-extracting
# kernel image is copied from FIS partition 3 (@ 0x50060000) to 0x01d00000
# by RedBoot.  RedBoot then copies the ramdisk from FIS partition 4
# (@ 0x50160000) to 0x01000000, and jumps to 0x01d00000.
#
# NOTE: While there is room for 3MB of gzboot+compressed kernel at the
# designationed location in SDRAM, there is only 1MB of space available
# in FIS partition 3!
#

PLATFORM= NSLU2
RELOC= 0x01d00000
MAXIMAGESIZE= 0		# concatenate
LOADADDR= 0x00200000

CPPFLAGS+= -DCONSPEED=115200
CPPFLAGS+= -DCONADDR=0xc8000000UL

CPUFLAGS= -mcpu=xscale

LDSCRIPT= ${.CURDIR}/ldscript

SRCS+=	nslu2.c ns16550.c ixp425_mem.c

.include "${S}/arch/evbarm/stand/gzboot/Makefile.gzboot"
