#	$NetBSD: lint.fs,v 1.1 2006/04/26 18:36:25 rpaulo Exp $

# File systems
file-system 	FFS		# UFS
file-system 	EXT2FS		# second extended file system (linux)
file-system 	LFS		# log-structured file system
file-system 	MFS		# memory file system
file-system 	NFS		# Network File System client
file-system 	NTFS		# Windows/NT file system (experimental)
file-system 	CD9660		# ISO 9660 + Rock Ridge file system
file-system 	MSDOSFS		# MS-DOS file system
file-system 	FDESC		# /dev/fd
file-system 	KERNFS		# /kern
file-system 	NULLFS		# loopback file system
file-system 	OVERLAY		# overlay file system
file-system 	PORTAL		# portal filesystem (still experimental)
file-system 	PROCFS		# /proc
file-system 	UMAPFS		# NULLFS + uid and gid remapping
file-system 	UNION		# union file system
file-system	CODA		# Coda File System; also needs vcoda (below)
file-system	SMBFS		# experimental - CIFS; also needs nsmb (below)
file-system	PTYFS		# /dev/ptm support
file-system	TMPFS		# Efficient memory file-system
file-system	UDF		# OSTA UDF CD/DVD file-system

# pseudo-devices required by the file-systems
pseudo-device	nsmb		# SMB requester
pseudo-device	vcoda	4	# coda minicache <-> venus comm.

# File system options
options 	QUOTA		# UFS quotas
options 	FFS_EI		# FFS Endian Independent support
options 	SOFTDEP		# FFS soft updates support.
options 	UFS_DIRHASH	# UFS Large Directory Hashing - Experimental
options 	NFSSERVER	# Network File System server
options 	EXT2FS_SYSTEM_FLAGS # makes ext2fs file flags (append and
				    # immutable) behave as system flags.

# File system related pseudo-devices
pseudo-device	ccd	4	# concatenated/striped disk devices
pseudo-device	cgd	4	# cryptographic disk devices
pseudo-device	fss	4	# file system snapshot device
pseudo-device	md	1	# memory disk device (ramdisk)
pseudo-device	vnd		# disk-like interface to files
options 	VND_COMPRESSION		# compressed vnd(4)

# RAID subsystem
pseudo-device	raid	8	# RAIDframe disk driver
options 	RAID_AUTOCONFIG	# auto-configuration of RAID components
#options 	RF_INCLUDE_EVENODD=1
#options 	RF_INCLUDE_RAID5_RS=1
#options 	RF_INCLUDE_PARITYLOGGING=1
#options 	RF_INCLUDE_CHAINDECLUSTER=1
#options 	RF_INCLUDE_INTERDECLUSTER=1
#options 	RF_INCLUDE_PARITY_DECLUSTERING=1
#options 	RF_INCLUDE_PARITY_DECLUSTERING_DS=1


# ATA RAID configuration support
ld*	at ataraid? vendtype ? unit ?
pseudo-device	ataraid
