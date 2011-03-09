#! /bin/sh
#
# Copyright (c) 2011 The NetBSD Foundation, Inc.
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
# THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
# CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
# IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
# IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#
# This script exports a fresh copy of the window(1) sources from cvs and
# generates a distfile for them.  You can later upload the resulting
# distfile to the NetBSD ftp site and update the pkgsrc/misc/window package.
#
# To generate a distfile, tag the sources in this directory with a tag of
# the form window-YYYYMMDD and then run this script providing the same tag
# name as the first argument and a target directory as the second argument.
#
# Example:
# src/usr.bin/window$ cvs tag window-20110309
# src/usr.bin/window$ ./export.sh window-20110309 /tmp
#

set -e

ProgName="${0##*/}"

err() {
	echo "${ProgName}:" "${@}" 1>&2
	exit 1
}

usage_error() {
	echo "Usage: ${ProgName} tag-name target-directory"
	echo "Example: ${ProgName} window-20110309 /tmp"
	exit 1
}

main() {
	[ -f wwopen.c ] || err "Must be run from window's source directory"

	[ -d CVS ] || err "Must be run from window's CVS source directory"
	local root="$(cat CVS/Root)"

	[ ${#} -eq 2 ] || usage_error
	local tag="${1}"; shift
	local directory="${1}"; shift

	local distname="${tag}"

	cd "${directory}"
	cvs -d "${root}" export -r "${tag}" -d "${distname}" src/usr.bin/window
	rm -f "${distname}"/"${ProgName}"
	tar czf "${distname}.tar.gz" "${distname}"
	rm -rf "${distname}"
	cd -
}

main "${@}"
