#	$NetBSD: std.pmax,v 1.0 1995/04/28 03:10:41 jonathan Exp
# standard pica info

machine pica mips

prefix ../gnu/sys
cinclude "conf/files.softdep"
prefix

mainbus0 at root
cpu* at mainbus0

# set CPU architecture level for kernel target
options 	MIPS3

# Standard (non-optional) system "options"
options 	SWAPPAGER		# swap pager (anonymous and swap space)
options 	VNODEPAGER		# vnode pager (mapped files)
options 	DEVPAGER		# device pager (mapped devices)

# Standard exec-package options
options 	EXEC_ELF32		# native exec format
options 	EXEC_SCRIPT		# may be unsafe
