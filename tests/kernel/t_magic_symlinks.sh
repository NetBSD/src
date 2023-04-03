# $NetBSD: t_magic_symlinks.sh,v 1.4 2023/04/03 21:35:59 gutteridge Exp $
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
tmpdir="/tmp/test-magic-symlink"

init() {

	enabled=$(sysctl vfs.generic.magiclinks | awk '{print $3}')

	if [ $enabled -eq 0 ]; then
		sysctl -w vfs.generic.magiclinks=1 >/dev/null 2>&1
		echo "Initialized vfs.generic.magiclinks=1"
	fi

	mkdir "$tmpdir"
	echo "$enabled" > "$tmpdir/enabled"
}

clean() {

	enabled=$(cat "$tmpdir/enabled")

	if [ $enabled -eq 0 ]; then
		sysctl -w vfs.generic.magiclinks=$enabled >/dev/null 2>&1
		echo "Restored vfs.generic.magiclinks=$enabled"
	fi

	rm -rf $tmpdir
}

check() {

	init
	cd "$tmpdir"
	mkdir "$1"
	echo 1 > "$1/magic"
	ln -s "$2" "link"
	cd "link"

	if [ -z $(pwd | grep "$1") ]; then
		atf_fail "kernel does not handle magic symlinks properly"
	fi

	if [ ! $(cat "magic") -eq 1 ]; then
		atf_fail "kernel does not handle magic symlinks properly"
	fi
}

# @domainname
#
atf_test_case domainname cleanup
domainname_head() {
	atf_set "require.user" "root"
	atf_set "descr" "Check that @domainname magic symlinks work"
}

domainname_body() {
	check "$(domainname)" "@domainname"
}

domainname_cleanup() {
	clean
}

# @hostname
#
atf_test_case hostname cleanup
hostname_head() {
	atf_set "require.user" "root"
	atf_set "descr" "Check that @hostname magic symlinks work"
}

hostname_body() {
	check "$(hostname)" "@hostname"
}

hostname_cleanup() {
	clean
}

# @machine
#
atf_test_case machine cleanup
machine_head() {
	atf_set "require.user" "root"
	atf_set "descr" "Check that @machine magic symlinks work"
}

machine_body() {
	check "$(uname -m)" "@machine"
}

machine_cleanup() {
	clean
}

# @machine_arch
#
atf_test_case machine_arch cleanup
machine_arch_head() {
	atf_set "require.user" "root"
	atf_set "descr" "Check that @machine_arch magic symlinks work"
}

machine_arch_body() {
	check "$(uname -p)" "@machine_arch"
}

machine_arch_cleanup() {
	clean
}

# @ostype
#
atf_test_case ostype cleanup
ostype_head() {
	atf_set "require.user" "root"
	atf_set "descr" "Check that @ostype magic symlinks work"
}

ostype_body() {
	check "$(uname -s)" "@ostype"
}

ostype_cleanup() {
	clean
}

# @ruid
#
atf_test_case ruid cleanup
ruid_head() {
	atf_set "require.user" "root"
	atf_set "descr" "Check that @ruid magic symlinks work"
}

ruid_body() {
	check "$(id -ru)" "@ruid"
}

ruid_cleanup() {
	clean
}

# @uid
#
atf_test_case uid cleanup
uid_head() {
	atf_set "require.user" "root"
	atf_set "descr" "Check that @uid magic symlinks work"
}

uid_body() {
	check "$(id -u)" "@uid"
}

uid_cleanup() {
	clean
}

# @rgid
#
atf_test_case rgid cleanup
rgid_head() {
	atf_set "require.user" "root"
	atf_set "descr" "Check that @rgid magic symlinks work"
}

rgid_body() {
	check "$(id -rg)" "@rgid"
}

rgid_cleanup() {
	clean
}

# @gid
#
atf_test_case gid cleanup
gid_head() {
	atf_set "require.user" "root"
	atf_set "descr" "Check that @gid magic symlinks work"
}

gid_body() {
	check "$(id -g)" "@gid"
}

gid_cleanup() {
	clean
}

# realpath(1)
#
atf_test_case realpath cleanup
realpath_head() {
	atf_set "require.user" "root"
	atf_set "descr" "Check that realpath(1) agrees with the " \
		"kernel on magic symlink(7)'s (PR lib/55361)"
}

realpath_body() {

	check "$(uname -r)" "@osrelease"
	realpath "$tmpdir/link"

	if [ ! $? -eq 0 ]; then
		atf_expect_fail "PR lib/55361"
		atf_fail "realpath does not handle magic symlinks properly"
	fi
}

realpath_cleanup() {
	clean
}

atf_init_test_cases() {
	atf_add_test_case domainname
	atf_add_test_case hostname
	atf_add_test_case machine
	atf_add_test_case machine_arch
	atf_add_test_case ostype
	atf_add_test_case ruid
	atf_add_test_case uid
	atf_add_test_case rgid
	atf_add_test_case gid
	atf_add_test_case realpath
}
