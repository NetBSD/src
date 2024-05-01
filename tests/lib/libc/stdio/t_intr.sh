# $NetBSD: t_intr.sh,v 1.7 2024/05/01 11:40:25 gson Exp $
#
# Copyright (c) 2021 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Christos Zoulas.
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

DIR=$(atf_get_srcdir)
MAX=10000000
LMAX=1000000
BSIZE=128000
SSIZE=256000
TMOUT=20

h_test() {
	local avail=$( df -m . | awk '{if (int($4) > 0) print $4}' )
	# The test data are stored in triplicate: numbers.in, numbers.out,
	# and a temporary "stdout" file created by ATF.  Each line consists
	# of up to 7 digits and a newline for a total of 8 bytes.
	local need=$(( 3 * $MAX * 8 / 1000000 ))
	if [ $avail -lt $need ]; then
		atf_skip "not enough free space in working directory"
	fi

	"${DIR}/h_makenumbers" "$1" > numbers.in
	"${DIR}/h_intr" \
	    -p "$2" -a ${SSIZE} -b ${BSIZE} -t ${TMOUT} \
	    -c "dd of=numbers.out msgfmt=quiet" numbers.in
	atf_check -o "file:numbers.in" cat numbers.out
}

atf_test_case stdio_intr_ionbf
stdio_intr_ionbf_head()
{
	atf_set "descr" "Checks stdio EINTR _IONBF"
}
stdio_intr_ionbf_body()
{
	h_test ${MAX} IONBF
}

atf_test_case stdio_intr_iolbf
stdio_intr_iolbf_head()
{
	atf_set "descr" "Checks stdio EINTR _IOLBF"
}
stdio_intr_iolbf_body()
{
	h_test ${LMAX} IOLBF
}

atf_test_case stdio_intr_iofbf
stdio_intr_iofbf_head()
{
	atf_set "descr" "Checks stdio EINTR _IOFBF"
}
stdio_intr_iofbf_body()
{
	h_test ${LMAX} IOFBF
}

atf_init_test_cases()
{
	atf_add_test_case stdio_intr_ionbf
	atf_add_test_case stdio_intr_iolbf
	# flappy test; see fflush.c 1.19 to 1.24
	#atf_add_test_case stdio_intr_iofbf
}
