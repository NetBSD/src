#	$NetBSD: t_sets.sh,v 1.4 2024/01/30 16:57:32 martin Exp $
#
# Copyright (c) 2024 The NetBSD Foundation, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
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

check_mtree()
{
	local set=$1

	cd /
	atf_check -o empty -s eq:0 \
		mtree -e </etc/mtree/set."$set"
}

set_case()
{
	local set=$1

	eval "set_${set}_head() { atf_set descr \"/etc/mtree/set.${set}\"; }"
	eval "set_${set}_body() { check_mtree ${set}; }"
}

set_case base
set_case comp
set_case debug
set_case dtb
#set_case etc
set_case games
set_case gpufw
set_case man
set_case misc
set_case modules
set_case rescue
set_case tests
set_case text
set_case xbase
set_case xcomp
set_case xdebug
#set_case xetc
set_case xfont
set_case xserver

atf_init_test_cases()
{

	# base is always installed -- hard-code this in case we make a
	# mistake with the automatic set detection.
	atf_add_test_case set_base

	# Test all of the sets that are installed, except for some
	# special cases.
	for mtree in /etc/mtree/set.*; do
		set=${mtree#/etc/mtree/set.}
		case $set in
		base)	# Handled above already.
			continue
			;;
		dtb)
			# contents of this set go to the boot partition,
			# which may not be mounted during normal operation
			if [ ! -d /boot/dtb ]; then
				continue;
			fi
			;;
		etc|xetc)
			# etc and xetc have files that may be modified
			# on installation, and also contain log files,
			# so let's skip them for now.
			continue
			;;
		*)	;;
		esac
		atf_add_test_case set_"${set}"
	done
}
