#	$NetBSD: files.cats,v 1.2 2001/05/23 21:21:43 chris Exp $
#
# First try for arm-specific configuration info
#

maxpartitions	8
maxusers 2 8 64

# COMPAT_OLD_OFW for SHARKs
defopt	COMPAT_OLD_OFW

# Maintain Interrupt statistics
defopt	IRQSTATS

# X server support in console drivers
defopt	XSERVER

# Bootloader options (COMPAT... to be dropped ASAP)
#defopt	COMPAT_OLD_BOOTLOADER

# Architectures and core logic
defopt	EBSA285

define todservice {}

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
major	{wd = 16}

# ISA DMA glue
file	arch/arm32/isa/isadma_machdep.c		isadma

# XXX ISA joystick driver
device	joy
file	arch/arm32/isa/joy.c			joy needs-flag
attach	joy at isa with joy_isa
file	arch/arm32/isa/joy_isa.c		joy_isa
attach	joy at isapnp with joy_isapnp
file	arch/arm32/isa/joy_isapnp.c		joy_isapnp

# Memory disk driver
file	dev/md_root.c				md & memory_disk_hooks
major   {md = 18}

# RAIDframe
major	{raid = 71}

#
# Machine-independent SCSI/ATAPI drivers
#

include "dev/scsipi/files.scsipi"
major   {sd = 24}
major   {cd = 26}

# Generic MD files
file	arch/arm32/arm32/blockio.S
file	arch/arm32/arm32/bus_dma.c
file	arch/arm32/arm32/conf.c
file	arch/arm32/arm32/cpuswitch.S
file	arch/arm32/arm32/exception.S
file	arch/arm32/arm32/fault.c
file	arch/arm32/arm32/fusu.S
file	arch/arm32/arm32/intr.c
file	arch/arm32/arm32/machdep.c
file	arch/arm32/arm32/mem.c
file	arch/arm32/arm32/procfs_machdep.c	procfs
file	arch/arm32/arm32/setcpsr.S
file	arch/arm32/arm32/setstack.S
file	arch/arm32/arm32/spl.S
file	arch/arm32/arm32/stubs.c
file	arch/arm32/arm32/vm_machdep.c

file	arch/arm32/dev/bus_space_notimpl.S

# library functions

file	arch/arm/arm/disksubr.c			disk
file	arch/arm/arm/disksubr_acorn.c		disk
file	arch/arm/arm/disksubr_mbr.c		disk

# ARM FPE
file	arch/arm32/fpe-arm/armfpe_glue.S	armfpe
file	arch/arm32/fpe-arm/armfpe_init.c	armfpe
file	arch/arm32/fpe-arm/armfpe.s		armfpe

# ISA Plug 'n Play autoconfiguration glue.
file	arch/arm32/isa/isapnp_machdep.c		isapnp

# CATS boards have an EBSA285 based core with an ISA bus
file	arch/cats/isa/isa_machdep.c			isa & ebsa285

device	sysbeep
attach	sysbeep at pcppi with sysbeep_isa
file	arch/arm32/isa/sysbeep_isa.c			sysbeep_isa

device dsrtc: todservice
attach dsrtc at isa
file	arch/arm32/isa/dsrtc.c				dsrtc
# Machine-independent I2O drivers.
include "dev/i2o/files.i2o"

# PCI devices

#
# Include PCI config
#
include "dev/pci/files.pci"

# network devices MII bus
include "dev/mii/files.mii"

device	pcib: isabus
attach	pcib at pci
file	arch/arm32/pci/pcib.c			pcib

# XXX THE FOLLOWING BLOCK SHOULD GO INTO dev/pci/files.pci, BUT CANNOT
# XXX BECAUSE NOT 'lpt' IS DEFINED IN files.isa, RATHER THAN files.
# XXX (when the conf/files and files.isa bogons are fixed, this can
# XXX be fixed as well.)

attach	lpt at puc with lpt_puc
file	dev/pci/lpt_puc.c	lpt_puc

file	arch/arm32/pci/pciide_machdep.c	pciide

# Include USB stuff
include "dev/usb/files.usb"

include "dev/pckbc/files.pckbc"

# Include WSCONS stuff
include "dev/wscons/files.wscons"
include "dev/rasops/files.rasops"
include "dev/pckbc/files.pckbc"
