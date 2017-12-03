# 	$NetBSD: INSTALL.inc,v 1.1.18.2 2017/12/03 11:36:11 jdolecek Exp $

# DEBUG options turned on:

options		PIPE_SOCKETPAIR		# smaller, but slower pipe(2)

pseudo-device	md

makeoptions	NEED_MDSETIMAGE="yes"

no config	netbsd
config		netbsd root on md0a

# Enable the hooks used for initializing the root memory-disk.
options 	MEMORY_DISK_HOOKS
options 	MEMORY_DISK_IS_ROOT	# force root on memory disk
options 	MEMORY_DISK_SERVER=0	# no userspace memory disk support
## The ramdisk size must be kept in sync manually with the size of
## the `ramdisk' image (which is built in distrib/evbppc/ramdisk/ramdisk).
options 	MEMORY_DISK_ROOT_SIZE=6560	# size of memory disk, in blocks
options 	MEMORY_DISK_RBFLAGS=RB_SINGLE	# boot in single-user mode
