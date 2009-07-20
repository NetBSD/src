# $NetBSD: GENERIC.NAS,v 1.10 2009/07/20 11:46:17 nisimura Exp $
#
# machine description file for GENERIC.NAS
# 
# This machine description file is used to generate the default NetBSD
# kernel.  The generic kernel does not include all options, subsystems
# and device drivers, but should be useful for most applications.
#
# The machine description file can be customised for your specific
# machine to reduce the kernel size and improve its performance.
#
# For further information on compiling NetBSD kernels, see the config(8)
# man page.
#
# For further information on hardware support for this architecture, see
# the intro(4) man page.  For further information about kernel options
# for this architecture, see the options(4) man page.  For an explanation
# of each device driver in this file see the section 4 man page for the
# device.

include 	"arch/sandpoint/conf/std.sandpoint"

options 	INCLUDE_CONFIG_FILE	# embed config file in kernel binary

#ident 		"GENERIC.NAS-$Revision: 1.10 $"

maxusers	32

#options 	ALTIVEC		# Include AltiVec support

#options 	IPKDBUSERHACK	# Allow access to mapped memory via kgdb
#options 	DEBUG_BY_TOOLS	# Allow debugging from toolsnet
# HMAC key to be used for debugging sessions
#options 	IPKDBKEY="\"HMAC pass phrase text\""
#options 	IPKDBSECURE	# Allow debugging even when securelevel > 0
#makeoptions	DEBUG="-g"

#options 	DEBUG
options 	DIAGNOSTIC
options 	TRAP_PANICWAIT
options 	DDB
#options 	DDB_HISTORY_SIZE=100	# Enable history editing in DDB
options 	KTRACE
#options 	PMAPCHECK
#options 	PMAPDEBUG
#options 	SYMTAB_SPACE=300000
#options 	USERCONF	# userconf(4) support
#options	PIPE_SOCKETPAIR		# smaller, but slower pipe(2)
options 	SYSCTL_INCLUDE_DESCR	# Include sysctl descriptions in kernel

# Compatibility options
options 	COMPAT_NOMID	# NetBSD 0.8, 386BSD, and BSDI
options 	COMPAT_09	# NetBSD 0.9
options 	COMPAT_10	# NetBSD 1.0
options 	COMPAT_11	# NetBSD 1.1
options 	COMPAT_12	# NetBSD 1.2, 386BSD, and BSDI
options 	COMPAT_13	# NetBSD 1.3, 386BSD, and BSDI
options 	COMPAT_14	# NetBSD 1.4
options 	COMPAT_15	# NetBSD 1.5
options 	COMPAT_16	# NetBSD 1.6
options 	COMPAT_20	# NetBSD 2.0
options 	COMPAT_30	# NetBSD 3.0
options 	COMPAT_40	# NetBSD 4.0
options 	COMPAT_50	# NetBSD 5.0 compatibility.
options 	COMPAT_43	# 4.3BSD, 386BSD, and BSDI
#options 	COMPAT_386BSD_MBRPART	# recognize old partition ID
#options 	TCP_COMPAT_42	# 4.2BSD TCP/IP bug compat. Not recommended.

options 	COMPAT_LINUX	# binary compatibility with Linux
options		COMPAT_BSDPTY	# /dev/[pt]ty?? ptys.

options 	MSGBUFSIZE=65536

#options 	WAPBL

file-system 	FFS		# UFS
file-system 	EXT2FS		# second extended file system (linux)
#file-system 	LFS		# log-structured file system
file-system 	MFS		# memory file system
file-system 	NFS		# Network File System client
#file-system 	NTFS		# Windows/NT file system (experimental)
#file-system 	CD9660		# ISO 9660 + Rock Ridge file system
#file-system 	MSDOSFS		# MS-DOS file system
file-system 	FDESC		# /dev/fd
file-system 	KERNFS		# /kern
file-system 	NULLFS		# loopback file system
#file-system 	OVERLAY		# overlay file system
#file-system 	PORTAL		# portal filesystem (still experimental)
#file-system 	PROCFS		# /proc
#file-system 	UMAPFS		# NULLFS + uid and gid remapping
#file-system 	UNION		# union file system
#file-system	CODA		# Coda File System; also needs vcoda (below)
#file-system	SMBFS		# experimental - CIFS; also needs nsmb (below)
file-system	PTYFS		# /dev/ptm support
file-system	TMPFS		# Efficient memory file-system

# File system options
options 	QUOTA		# UFS quotas
#options 	FFS_EI		# FFS Endian Independent support
options 	WAPBL		# File system journaling support - Experimental
options 	NFSSERVER	# Network File System server
#options 	FFS_NO_SNAPSHOT	# No FFS snapshot support
#options 	EXT2FS_SYSTEM_FLAGS # makes ext2fs file flags (append and

options 	INET
#options 	TCP_DEBUG	# Record last TCP_NDEBUG packets with SO_DEBUG

#options 	ALTQ		# Manipulate network interfaces' output queues
#options 	ALTQ_BLUE	# Stochastic Fair Blue
#options 	ALTQ_CBQ	# Class-Based Queueing
#options 	ALTQ_CDNR	# Diffserv Traffic Conditioner
#options 	ALTQ_FIFOQ	# First-In First-Out Queue
#options 	ALTQ_FLOWVALVE	# RED/flow-valve (red-penalty-box)
#options 	ALTQ_HFSC	# Hierarchical Fair Service Curve
#options 	ALTQ_LOCALQ	# Local queueing discipline
#options 	ALTQ_PRIQ	# Priority Queueing
#options 	ALTQ_RED	# Random Early Detection
#options 	ALTQ_RIO	# RED with IN/OUT
#options 	ALTQ_WFQ	# Weighted Fair Queueing

options 	NFS_BOOT_BOOTPARAM,NFS_BOOT_BOOTP

config		netbsd	root on ? type ?

# network psuedo-devices
pseudo-device	bpfilter		# packet filter
pseudo-device	vlan			# IEEE 802.1q encapsulation
#pseudo-device	pppoe			# PPP over Ethernet (RFC 2516)
#pseudo-device	bridge			# simple inter-network bridging
#options	BRIDGE_IPF		# bridge uses IP/IPv6 pfil hooks too
#pseudo-device	ipfilter
pseudo-device	loop

# miscellaneous pseudo-devices
pseudo-device	rnd			# /dev/random and in-kernel generator
pseudo-device	vnd		
#options 	VND_COMPRESSION		# compressed vnd(4)
#pseudo-device	cgd		4	# cryptographic disk devices
pseudo-device	pty
pseudo-device	clockctl		# user control of clock subsystem
pseudo-device	ksyms			# /dev/ksyms
#pseudo-device	pf			# PF packet filter
#pseudo-device	pflog			# PF log if

#
# device
#
options 	RTC_OFFSET=0	# hardware clock is this many mins. west of GMT

#options 	PCIVERBOSE	# verbose PCI device messages
#options 	PCI_CONFIG_DUMP	# verbosely dump PCI config space
#options 	PCI_NETBSD_CONFIGURE	# Have NetBSD configure PCI I/O & Mem

mainbus0 at root

cpu*	at mainbus0
pci*	at mainbus0 bus ?

pchb*	at pci? dev ? function ?	# PCI host bridge

eumb*	at mainbus0
com0	at eumb? unit 1			# console is at 0x4600
com1	at eumb? unit 0
#com0	at eumb? unit 0			# standard case
#com1	at eumb? unit 1
ociic*	at eumb?
iic*	at ociic?
rs5c372rtc* at iic? addr 0x32

# PCI IDE controllers - see pciide(4) for supported hardware.
cmdide*		at pci? dev ? function ?	# CMD tech IDE controllers
iteide*		at pci? dev ? function ?	# IT Express IDE controllers
satalink*	at pci? dev ? function ?	# SiI SATALink controllers

# ATA (IDE) bus support
atabus* at ata?

# IDE drives
wd*		at atabus? drive ? flags 0x0000	# the drives themselves

# PCI network interfaces
tlp*	at pci? dev ? function ?	# DEC Tulip and similar
re*	at pci? dev ? function ?	# Realtek 8139C+/8169/8169S/8110S
mskc*	at pci? dev ? function ?	# Marvell Yukon 2 Gigabit Ethernet
msk*	at mskc?			# Marvell Yukon 2 Gigabit Ethernet
wm*	at pci? dev ? function ?	# Intel 8254x gigabit
fxp*	at pci? dev ? function ?	# Intel EtherExpress PRO 10+/100B

inphy*	at mii? phy ?			# Intel 82555 PHYs
nsphy*	at mii? phy ?			# NS83840 PHYs
rgephy* at mii? phy ?			# Realtek 8169S/8110S internal PHYs
makphy* at mii? phy ?			# Marvell Semiconductor 88E1000 PHYs
ukphy*	at mii? phy ?			# generic unknown PHYs

# USB Controller and Devices
ehci*	at pci?	dev ? function ?	# Enhanced Host Controller
ohci*	at pci?	dev ? function ?	# Open Host Controller

# USB bus support
usb*	at ehci?
usb*	at ohci?

# USB Hubs
uhub*	at usb?
uhub*	at uhub? port ?

# USB Mass Storage
umass*	at uhub? port ? configuration ? interface ?
wd*	at umass?

scsibus* at umass?
sd*	at scsibus? target ? lun ?	# SCSI disk drives

# Veriexec
#
# a pseudo device needed for veriexec
#pseudo-device	veriexec		1
#
# Uncomment the fingerprint methods below that are desired. Note that
# removing fingerprint methods will have almost no impact on the kernel
# code size.
#
#options VERIFIED_EXEC_FP_RMD160
#options VERIFIED_EXEC_FP_SHA256
#options VERIFIED_EXEC_FP_SHA384
#options VERIFIED_EXEC_FP_SHA512
#options VERIFIED_EXEC_FP_SHA1
#options VERIFIED_EXEC_FP_MD5
