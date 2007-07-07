#	$NetBSD: files.arc,v 1.59 2007/07/07 05:13:14 tsutsui Exp $
#	$OpenBSD: files.arc,v 1.21 1999/09/11 10:20:20 niklas Exp $
#
# maxpartitions must be first item in files.${ARCH}
#
maxpartitions 16

maxusers 2 8 64

##
##	Platform support option header and files
##

defflag	opt_platform.h			PLATFORM_ACER_PICA_61
					PLATFORM_DESKTECH_ARCSTATION_I
					PLATFORM_DESKTECH_TYNE
					PLATFORM_MICROSOFT_JAZZ
					PLATFORM_NEC_J96A
					PLATFORM_NEC_JC94
					PLATFORM_NEC_R94
					PLATFORM_NEC_R96
					PLATFORM_NEC_RAX94
					PLATFORM_NEC_RD94
					PLATFORM_SNI_RM200PCI

file	arch/arc/arc/c_isa.c		platform_desktech_arcstation_i |
					platform_desktech_tyne
file	arch/arc/arc/c_jazz_eisa.c	platform_acer_pica_61 |
					platform_microsoft_jazz |
					platform_nec_j96a |
					platform_nec_r94 |
					platform_nec_r96
file	arch/arc/arc/c_magnum.c		platform_acer_pica_61 |
					platform_microsoft_jazz
file	arch/arc/arc/c_nec_eisa.c	platform_nec_j96a |
					platform_nec_r94 |
					platform_nec_r96
file	arch/arc/arc/c_nec_jazz.c	platform_nec_j96a |
					platform_nec_jc94 |
					platform_nec_r94 |
					platform_nec_r96 |
					platform_nec_rax94 |
					platform_nec_rd94
file	arch/arc/arc/c_nec_pci.c	platform_nec_jc94 |
					platform_nec_rax94 |
					platform_nec_rd94

file	arch/arc/arc/p_acer_pica_61.c	platform_acer_pica_61
file	arch/arc/arc/p_dti_arcstation.c	platform_desktech_arcstation_i
file	arch/arc/arc/p_dti_tyne.c	platform_desktech_tyne
file	arch/arc/arc/p_ms_jazz.c	platform_microsoft_jazz
file	arch/arc/arc/p_nec_j96a.c	platform_nec_j96a
file	arch/arc/arc/p_nec_jc94.c	platform_nec_jc94
file	arch/arc/arc/p_nec_r94.c	platform_nec_r94
file	arch/arc/arc/p_nec_r96.c	platform_nec_r96
file	arch/arc/arc/p_nec_rax94.c	platform_nec_rax94
file	arch/arc/arc/p_nec_rd94.c	platform_nec_rd94
file	arch/arc/arc/p_sni_rm200pci.c	platform_sni_rm200pci

file	arch/arc/arc/platconf.c
file	arch/arc/arc/platform.c

##
##	Required files
##

file	arch/arc/arc/autoconf.c
file	arch/arc/arc/disksubr.c
file	arch/arc/arc/machdep.c
#file	arch/arc/arc/minidebug.c
file	arch/arc/arc/timer.c
file	arch/arc/arc/interrupt.c
file	arch/arc/arc/bus_space.c
file	arch/arc/arc/bus_space_sparse.c
file	arch/arc/arc/bus_space_large.c
file	arch/arc/arc/bus_dma.c
file	arch/arc/arc/wired_map_machdep.c

file	arch/arc/arc/arcbios.c

file	arch/mips/mips/mips3_clock.c
file	arch/mips/mips/softintr.c

##
##	Machine-independent ATAPI drivers
##
include "dev/ata/files.ata"

# Raster operations
include "dev/rasops/files.rasops"
include "dev/wsfont/files.wsfont"
include "dev/wsfb/files.wsfb"

#
# "Workstation Console" glue.
#
include "dev/wscons/files.wscons"

include "dev/pckbport/files.pckbport"

#
#	System BUS types
#
device mainbus { }			# no locators
attach mainbus at root
file	arch/arc/arc/mainbus.c	mainbus

#	Our CPU configurator
device cpu				# not optional
attach cpu at mainbus
file arch/arc/arc/cpu.c			cpu

#
#	Magnum and Jazz-Internal bus autoconfiguration devices
#
device	jazzio {}
attach	jazzio at mainbus		# optional
file	arch/arc/jazz/jazzio.c		jazzio
file	arch/arc/jazz/jazzdmatlb.c	# XXX jazzio
file	arch/arc/jazz/bus_dma_jazz.c	# XXX jazzio

#
#	ISA Bus bridge
#
define	isabr
file	arch/arc/isa/isabus.c		isabr

#device	jazzisabr {} : eisabus, isabus, isabr
device	jazzisabr {} : isabus, isabr
attach	jazzisabr at mainbus
file	arch/arc/jazz/jazzisabr.c	jazzisabr

device	arcsisabr {} : isabus, isabr	# PLATFORM_DESKTECH_ARCSTATION_I
attach	arcsisabr at mainbus
file	arch/arc/dti/arcsisabr.c	arcsisabr
file	arch/arc/isa/isadma_bounce.c	arcsisabr

device	tyneisabr {} : isabus, isabr	# PLATFORM_DESKTECH_TYNE
attach	tyneisabr at mainbus
file	arch/arc/dti/tyneisabr.c	tyneisabr

# ISA Plug 'n Play devices
file	arch/arc/isa/isapnp_machdep.c	isapnp

#
#	NEC RISCstation PCI host bridge
#
device	necpb: pcibus
attach	necpb at mainbus		# optional
file	arch/arc/pci/necpb.c		necpb

#	Ethernet chip on Jazz-Internal bus
# XXX device declaration of MI sonic should be moved into sys/conf/files
device	sn: ifnet, ether, arp
file	dev/ic/dp83932.c		sn
attach	sn at jazzio with sn_jazzio
file	arch/arc/jazz/if_sn_jazzio.c	sn_jazzio

#
# Machine-independent I2O drivers.
#
include "dev/i2o/files.i2o"

#	Use machine independent SCSI driver routines
include	"dev/scsipi/files.scsipi"

#	Symbios 53C94 SCSI interface driver on Jazz-Internal bus
device	asc: scsi, ncr53c9x
attach	asc at jazzio
file	arch/arc/jazz/asc.c		asc	needs-flag

#	Symbios 53C710 SCSI interface driver on Jazz-Internal bus
attach	osiop at jazzio with osiop_jazzio
file	arch/arc/jazz/osiop_jazzio.c	osiop_jazzio

#	NCR53C700 SCSI I/O processor on Jazz-Internal bus
attach	oosiop at jazzio with oosiop_jazzio
file	arch/arc/jazz/oosiop_jazzio.c		oosiop_jazzio

#	Floppy disk controller on Jazz-internal bus
device	fdc {drive = -1}
file	arch/arc/jazz/fd.c		fdc	needs-flag

attach	fdc at jazzio with fdc_jazzio
file	arch/arc/jazz/fdc_jazzio.c	fdc_jazzio

device	fd: disk
attach	fd at fdc

#	bus independent raster console glue
device	rasdisplay: wsemuldisplaydev, pcdisplayops
file	arch/arc/dev/rasdisplay.c	rasdisplay

#	raster console glue on Jazz-Internal bus
attach	rasdisplay at jazzio with rasdisplay_jazzio
file	arch/arc/jazz/rasdisplay_jazzio.c rasdisplay_jazzio needs-flag

#	VGA display driver on Jazz-Internal bus
attach	vga at jazzio with vga_jazzio
file	arch/arc/jazz/vga_jazzio.c	vga_jazzio needs-flag

#	PC keyboard controller on Jazz-Internal bus
attach	pckbc at jazzio with pckbc_jazzio
file	arch/arc/jazz/pckbc_jazzio.c	pckbc_jazzio needs-flag

#
#	Stock ISA bus support
#
define	pcmcia {}			# XXX dummy decl...

include	"dev/pci/files.pci"
#include	"dev/eisa/files.eisa"
include	"dev/isa/files.isa"
#file	arch/arc/eisa/eisa_machdep.c	eisa

#	Interval timer, must have one..
device	timer
attach	timer at jazzio with timer_jazzio
attach	timer at isa with timer_isa
file	arch/arc/jazz/timer_jazzio.c	timer & timer_jazzio needs-flag
file	arch/arc/isa/timer_isa.c	timer & timer_isa needs-flag

#	Real time clock, must have one..
device	mcclock: mc146818
attach	mcclock at jazzio with mcclock_jazzio
attach	mcclock at isa with mcclock_isa
file	arch/arc/jazz/mcclock_jazzio.c	mcclock & mcclock_jazzio needs-flag
file	arch/arc/isa/mcclock_isa.c	mcclock & mcclock_isa needs-flag

#	Console driver on PC-style graphics
device	pc: tty
file	arch/arc/dev/pccons.c		(pc | opms) &
					(pc_jazzio | pc_isa |
					 opms_jazzio | opms_isa) needs-flag
attach	pc at jazzio with pc_jazzio
file	arch/arc/jazz/pccons_jazzio.c	pc_jazzio | opms_jazzio
attach	pc at isa with pc_isa
file	arch/arc/isa/pccons_isa.c	pc_isa

# PS/2-style mouse
device	opms: tty
file	arch/arc/dev/opms.c		opms
attach	opms at jazzio with opms_jazzio
file	arch/arc/jazz/opms_jazzio.c	opms_jazzio
attach	opms at isa with opms_isa
file	arch/arc/isa/opms_isa.c		opms_isa

#	BusLogic BT-445C VLB SCSI Controller. Special on TYNE local bus.
device	btl: scsi
attach	btl at isa
file	arch/arc/dti/btl.c		btl needs-flag

#	NS16450/16550 Serial line driver
attach	com at jazzio with com_jazzio
file	arch/arc/jazz/com_jazzio.c	com & com_jazzio

#	Parallel printer port driver
attach	lpt at jazzio with lpt_jazzio
file	arch/arc/jazz/lpt_jazzio.c	lpt & lpt_jazzio


#
#	PCI Bus support
#

# PCI VGA display driver
device	pcivga: tty
attach	pcivga at pci
file	arch/arc/pci/pci_vga.c		pcivga

#
# Specials.
#
# memory disk for installation
file dev/md_root.c			memory_disk_hooks

# USB
include "dev/usb/files.usb"

#
#	Common files
#

file	dev/cons.c
#file	dev/cninit.c
#file	netinet/in_cksum.c

include "arch/arc/conf/majors.arc"
