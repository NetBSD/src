#!/usr/bin/awk -
#
#	$NetBSD: MAKEDEV.awk,v 1.2 2003/10/15 19:43:00 jdolecek Exp $
#
# Copyright (c) 2003 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Jaromir Dolecek.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#        This product includes software developed by the NetBSD
#        Foundation, Inc. and its contributors.
# 4. Neither the name of The NetBSD Foundation nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

# Script to generate platform MAKEDEV script from MI template, MD
# MAKEDEV.conf and MD/MI major lists
#
# Uses environment variables MACHINE/MACHINE_ARCH to select
# appropriate files, and NETBSDSRCDIR to get root of source tree.

BEGIN {
	# top of source tree, used to find major number list in kernel
	# sources
	top = ENVIRON["NETBSDSRCDIR"]
	if (!top)
		top = ".."
	top = top "/sys/"
	if (system("test -d '" top "'") != 0) {
		print "ERROR: didn't find top of kernel tree ('" top "' not a directory)" > "/dev/stderr"
		exit 1
	}

	machine = ENVIRON["MACHINE"]
	if (!machine)
		machine = "i386"	# XXX for testing
	maarch = ENVIRON["MACHINE_ARCH"]

	# file with major definitions
	majors[0] = "conf/majors"
	if (maarch == "arm32")
		majors[1] = "arch/arm/conf/majors.arm32";
	else if (machine == "evbsh5") {
		majors[1] = "arch/evbsh5/conf/majors.evbsh5";
		majors[2] = "arch/sh5/conf/majors.sh5";
	} else
		majors[1] = "arch/" machine "/conf/majors." machine;

	# process all files with majors and fill the chr[] and blk[]
	# arrays, used in template processing
	for(m in majors) {
		file = top majors[m]
		while (getline < file) {
			if ($1 == "device-major") {
				if ($3 == "char") {
					chr[$2] = $4
					if ($5 == "block")
						blk[$2] = $6
				} else if ($3 == "block")
					blk[$2] = $4
			}
		}
	}

	# read MD config file, and determine disk partitions
	# and MD device list
	cfgfile = "etc." machine "/MAKEDEV.conf"
	MDDEV = 0		# MD device targets
	MKDISK = ""		# routine to create disk devices
	while (getline < cfgfile) {
		if ($1 ~ "^DISKPARTITIONS=") {
			sub(".*=[ \t]*", "")
			MKDISK = "makedisk_p" $0
		} else if (MDDEV) {
			if (MDDEV == 1)
				MDDEV = $0
			else
				MDDEV = MDDEV "\n" $0
		} else if ($1 ~ "^MD_DEVICES=")
			MDDEV = 1
	}

	# initially no substitutions
	devsubst = 0
	deventry = ""
}

/%MI_DEVICES_BEGIN%/ {
	devsubst = 1;
	next
}

/%MI_DEVICES_END%/ {
	devsubst = 0;
	next
}

{
	sub("^%MD_DEVICES%", MDDEV)
	sub("%MKDISK%", MKDISK)

	# if device substitutions are not active, do nothing more
	if (!devsubst) {
		print
		next
	}
}

# first line of device entry
/^[a-z].*\)$/ {
	if (length(deventry) > 0) {
		# We have a previous entry to print. Replace all known
		# character and block devices. If no unknown character
		# or block device definition remains within the entry,
		# print it to output, otherwise scrap it.
		for(c in chr)
			gsub("%" c "_chr%", chr[c], deventry)
		for(b in blk)
			gsub("%" b "_blk%", blk[b], deventry)

		if (deventry !~ "%[a-z]*_chr%" && deventry !~ "%[a-z]*_blk%")
			print deventry
	}
	deventry = $0
	next
}

# template line within device substitution section - just keep appending
# to the current entry
{
	deventry = deventry "\n" $0
}
