#	$NetBSD: files.arc,v 1.20 2000/05/29 10:17:44 soda Exp $
#	$OpenBSD: files.arc,v 1.21 1999/09/11 10:20:20 niklas Exp $
#
# maxpartitions must be first item in files.${ARCH}
#
maxpartitions 16

maxusers 2 8 64

#	Required files

file	arch/arc/arc/autoconf.c
file	arch/arc/arc/conf.c
file	arch/arc/arc/disksubr.c
file	arch/arc/dev/dma.c
file	arch/arc/arc/machdep.c
#file	arch/arc/arc/minidebug.c
file	arch/arc/arc/arc_trap.c

file	arch/arc/arc/arcbios.c

##
##	Machine-independent ATAPI drivers 
##
include "dev/ata/files.ata"
major	{ wd = 4 }

#
# "Workstation Console" glue.
#
include "dev/wscons/files.wscons"

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
#	Magnum and PICA bus autoconfiguration devices
#
device	pica {}
attach	pica at mainbus			# optional
file	arch/arc/pica/picabus.c		pica

#
#	ALGOR bus autoconfiguration devices
#
device	algor {}
attach	algor at mainbus		# optional
file	arch/arc/algor/algorbus.c	algor

#
#	ISA Bus bridge
#
device	isabr {} : isabus
attach	isabr at mainbus		# optional
file	arch/arc/isa/isabus.c		isabr

#
#	PCI Bus bridge
#
device	pbcpcibr {} : pcibus
attach	pbcpcibr at mainbus		# optional
file	arch/arc/pci/pbcpcibus.c	pbcpcibr

#	Ethernet chip on PICA bus
device	sn: ifnet, ether, arp
attach	sn at pica
file	arch/arc/dev/if_sn.c		sn

#	Use machine independent SCSI driver routines
include	"dev/scsipi/files.scsipi"
major	{sd = 0}
major	{cd = 3}

#	Symbios 53C94 SCSI interface driver on PICA bus
device	asc: scsi
attach	asc at pica
file	arch/arc/dev/asc.c		asc

#	Floppy disk controller on PICA bus
device	fdc {drive = -1}
attach	fdc at pica
device	fd: disk
attach	fd at fdc
file	arch/arc/dev/fd.c		fdc	needs-flag
major	{fd = 7}

#
#	Stock ISA bus support
#
define	pcmcia {}			# XXX dummy decl...

include	"dev/pci/files.pci"
include	"dev/isa/files.isa"

#	Real time clock, must have one..
device	aclock
attach	aclock at pica with aclock_pica
attach	aclock at isa with aclock_isa
attach	aclock at algor with aclock_algor
file	arch/arc/arc/clock.c	aclock & (aclock_isa | aclock_pica | aclock_algor) needs-flag
file	arch/arc/arc/clock_mc.c	aclock & (aclock_isa | aclock_pica | aclock_algor) needs-flag

#	Console driver on PC-style graphics
device	pc: tty
attach	pc at pica with pc_pica
attach	pc at isa with pc_isa
device	opms: tty
attach	opms at pica
file	arch/arc/dev/pccons.c		pc & (pc_pica | pc_isa)	needs-flag

#	BusLogic BT-445C VLB SCSI Controller. Special on TYNE local bus.
device	btl: scsi
attach	btl at isa
file	arch/arc/dti/btl.c		btl

#	NS16450/16550 Serial line driver
attach	com at pica with com_pica
attach	com at algor with com_algor
file	arch/arc/dev/com_lbus.c		com & (com_pica | com_algor)

# Game adapter (joystick)
device	joy
attach	joy at isa
file	arch/arc/isa/joy.c		joy needs-flag

# National Semiconductor DS8390/WD83C690-based boards
# (WD/SMC 80x3 family, SMC Ultra [8216], 3Com 3C503, NE[12]000, and clones)
# XXX conflicts with other ports; can't be in files.isa
## XXX: should fix conflict with files.isa
#device	ed: ether, ifnet
#attach	ed at isa with ed_isa
#attach	ed at pcmcia with ed_pcmcia
#file	dev/isa/if_ed.c			ed & (ed_isa | ed_pcmcia) needs-flag

#	Parallel printer port driver
attach	lpt at pica with lpt_pica
attach	lpt at algor with lpt_algor
file	arch/arc/dev/lpt_lbus.c		lpt & (lpt_pica | lpt_algor)


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
file arch/arc/dev/md_root.c		memory_disk_hooks
major {md = 8}

# RAIDframe
major {raid = 9}

#
#	Common files
#

file	dev/cons.c
#file	dev/cninit.c
#file	netinet/in_cksum.c
#file	netns/ns_cksum.c			ns
