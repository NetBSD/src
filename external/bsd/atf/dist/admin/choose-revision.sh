#! /bin/sh
#
# Automated Testing Framework (atf)
#
# Copyright (c) 2009 The NetBSD Foundation, Inc.
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

set -e

Prog_Name=${0##*/}

#
# err message
#
err() {
    echo "${Prog_Name}: ${@}" 1>&2
    exit 1
}

#
# do_cp
#
# Verbose copy.
#
do_cp() {
    echo "${Prog_Name}: cp -p ${1} ${2}"
    cp -p ${1} ${2}
}

#
# main
#
# Entry point.
#
main() {
    [ ${#} -eq 3 ] || err "Invalid number of arguments"
    mtnfile=${1}
    distfile=${2}
    tgtfile=${3}

    if [ -f ${mtnfile} ]; then
        cmp -s ${mtnfile} ${tgtfile} || do_cp ${mtnfile} ${tgtfile}
    else
        cmp -s ${distfile} ${tgtfile} || do_cp ${distfile} ${tgtfile}
    fi
}

main "${@}"

# vim: syntax=sh:expandtab:shiftwidth=4:softtabstop=4

