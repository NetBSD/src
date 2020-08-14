# $NetBSD: t_cpuctl.sh,v 1.5 2020/08/14 05:22:25 martin Exp $
#
# Copyright (c) 2020 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Jukka Ruohonen.
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
tmp="/tmp/cpuctl.txt"

setcpu() {

	ncpu=$(sysctl -n hw.ncpu)

	if [ $ncpu -eq 1 ]; then
		atf_pass
	fi

	while [ $ncpu -gt 1 ]; do

		cpuid=$(( $ncpu - 1 ))
		cpuctl $1 $cpuid >/dev/null 2>&1

		if [ ! $? -eq 0 ]; then
			$2 $3
		fi

		ncpu=$(( $ncpu - 1 ))
	done

	# Additional check that interrupts cannot be
	# disabled for the primary CPU (PR kern/45117).
	#
	cpuctl nointr 0 >/dev/null 2>&1

	if [ $? -eq 0 ]; then
		$2 $3
	fi
}

clean() {

	i=0

	while read line; do

		i=$(( $i + 1 ))

		if [ $i -lt 3 ]; then
			continue
		fi

		cpuid=$(echo $line | awk '{print $1}')
		online=$(echo $line | awk '{print $3}')
		intr=$(echo $line | awk '{print $4}')

		cpuctl $online $cpuid
		cpuctl $intr $cpuid

	done < $tmp

	rm $tmp
}

# ncpu.
#
atf_test_case ncpu
ncpu_head() {
	atf_require_prog cpuctl
	atf_set "descr" "Test that cpuctl(8) returns the " \
			"same number of CPUs as sysctl(8)"
}

ncpu_body() {

	lst=$(cpuctl list | wc -l)
	ncpu=$(( $lst - 2 ))

	if [ $ncpu -eq 1 ]; then
		atf_pass
	fi

	if [ $(sysctl -n hw.ncpu) -eq $ncpu ]; then
		atf_pass
	fi

	atf_fail "Different number of CPUs"
}

# err
#
atf_test_case err cleanup
err_head() {
	atf_require_prog cpuctl
	atf_set "require.user" "root"
	atf_set "descr" "Test invalid parameters to cpuctl(8)"
}

err_body() {

	cpuctl list > $tmp
	ncpu=$(sysctl -n hw.ncpu)

	atf_check -s exit:1 -e ignore \
		-o empty -x cpuctl identify -1

	atf_check -s exit:1 -e ignore \
		-o empty -x cpuctl offline -1

	atf_check -s exit:1 -e ignore \
		-o empty -x cpuctl nointr -1

	atf_check -s exit:1 -e ignore \
		-o empty -x cpuctl identify $(( $ncpu + 1 ))

	atf_check -s exit:1 -e ignore \
		  -o empty -x cpuctl offline $(( $ncpu + 1 ))

	atf_check -s exit:1 -e ignore \
		-o empty -x cpuctl nointr $(( $ncpu + 1 ))
}

err_cleanup() {
	clean
}

# identify
#
atf_test_case identify
identify_head() {
	atf_require_prog cpuctl
	atf_set "descr" "Test that cpuctl(8) identifies at least " \
			"something without segfaulting (PR bin/54220)"
}

identify_body() {

	ncpu=$(sysctl -n hw.ncpu)

	while [ $ncpu -gt 0 ]; do
		cpuid=$(( $ncpu - 1 ))
		atf_check -s exit:0 -o not-empty -x cpuctl identify $cpuid
		ncpu=$(( $ncpu - 1 ))
	done

	atf_pass
}

# offline
#
atf_test_case offline cleanup
offline_head() {
	atf_require_prog cpuctl
	atf_set "require.user" "root"
	atf_set "descr" "Test setting CPUs offline"
}

offline_body() {

	cpuctl list > $tmp
	setcpu "offline" atf_fail "error in setting a CPU offline"

	# Additional check that the boot processor cannot be
	# set offline, as noted in the cpuctl(8) manual page.
	#
	cpuctl offline 0 >/dev/null 2>&1

	if [ $? -eq 0 ]; then
		$2 $3
	fi
}

offline_cleanup() {
	clean
}

atf_test_case offline_perm
offline_perm_head() {
	atf_require_prog cpuctl
	atf_set "require.user" "unprivileged"
	atf_set "descr" "Test setting CPUs offline as a user"
}

offline_perm_body() {
	setcpu "offline" atf_pass
}

# nointr
#
atf_test_case nointr cleanup
nointr_head() {
	atf_require_prog cpuctl
	atf_set "require.user" "root"
	atf_set "descr" "Test disabling interrupts for CPUs"
}

nointr_body() {
	cpuctl list > $tmp
	setcpu "nointr" atf_fail "error in disabling interrupts"
}

nointr_cleanup() {
	clean
}

atf_test_case nointr_perm
nointr_perm_head() {
	atf_require_prog cpuctl
	atf_set "require.user" "unprivileged"
	atf_set "descr" "Test disabling interrupts as a user"
}

nointr_perm_body() {
	setcpu "nointr" atf_pass
}

atf_init_test_cases() {
	atf_add_test_case ncpu
	atf_add_test_case err
	atf_add_test_case identify
	atf_add_test_case offline
	atf_add_test_case offline_perm
	atf_add_test_case nointr
	atf_add_test_case nointr_perm
}
