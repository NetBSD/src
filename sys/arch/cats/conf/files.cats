#	$NetBSD: files.cats,v 1.33.8.1 2007/04/10 13:22:54 ad Exp $
#
# CATS-specific configuration info
#

maxpartitions	8
maxusers 2 8 64

# Maintain Interrupt statistics
defflag	IRQSTATS

# X server support in console drivers
defflag	XSERVER

# ABLE booting ELF kernels
defflag ABLEELF

#
# ISA and mixed ISA+EISA or ISA+PCI drivers
#
include "dev/isa/files.isa"

# Include arm32 footbridge
include "arch/arm/conf/files.footbridge"

#
# Machine-independent ATA drivers
#
include "dev/ata/files.ata"

# ISA DMA glue
file	arch/arm/footbridge/isa/isadma_machdep.c	isadma

# Memory disk driver
file	dev/md_root.c				md & memory_disk_hooks

#
# Machine-independent SCSI/ATAPI drivers
#

include "dev/scsipi/files.scsipi"

# Generic MD files
file	arch/cats/cats/autoconf.c
file	arch/cats/cats/cats_machdep.c

# library functions

file	arch/arm/arm/disksubr.c			disk
file	arch/arm/arm/disksubr_acorn.c		disk
file	arch/arm/arm/disksubr_mbr.c		disk

# ISA Plug 'n Play autoconfiguration glue.
file	arch/arm/footbridge/isa/isapnp_machdep.c	isapnp

# ISA support.
file	arch/arm/footbridge/isa/isa_io.c		isa
file	arch/arm/footbridge/isa/isa_io_asm.S		isa

# CATS boards have an EBSA285 based core with an ISA bus
file	arch/arm/footbridge/isa/isa_machdep.c		isa

device	sysbeep
attach	sysbeep at pcppi with sysbeep_isa
file	arch/arm/footbridge/isa/sysbeep_isa.c		sysbeep_isa

device ds1687rtc
attach ds1687rtc at isa
file	arch/arm/footbridge/isa/dsrtc.c			ds1687rtc

# Machine-independent I2O drivers.
include "dev/i2o/files.i2o"

# generic fb driver
include "dev/wsfb/files.wsfb"

# PCI devices

#
# Include PCI config
#
include "dev/pci/files.pci"

device	pcib: isabus
attach	pcib at pci
file	arch/cats/pci/pcib.c			pcib

file	arch/cats/pci/pciide_machdep.c	pciide_common

# Include WSCONS stuff
include "dev/wscons/files.wscons"
include "dev/rasops/files.rasops"
include "dev/wsfont/files.wsfont"
include "dev/pckbport/files.pckbport"

# Include USB stuff
include "dev/usb/files.usb"

include "arch/arm/conf/majors.arm32"
