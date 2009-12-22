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

#
# Modifies a revision file to generate a new one that can be redistributed
# as part of the source tree.
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
# generate_h infile outfile
#
generate_h() {
    cp ${1} ${2}
    echo '#define PACKAGE_REVISION_CACHED 1' >>${2}
}

#
# main args
#
# Entry point.
#
main() {
    fmt=
    infile=
    outfile=
    while getopts :f:i:o: arg; do
        case ${arg} in
            f)
                fmt=${OPTARG}
                ;;
            i)
                infile=${OPTARG}
                ;;
            o)
                outfile=${OPTARG}
                ;;
            *)
                err "Unknown option ${arg}"
                ;;
        esac
    done
    [ -n "${fmt}" ] || \
        err "Must specify an output format with -f"
    [ -n "${infile}" ] || \
        err "Must specify an input file with -i"
    [ -n "${outfile}" ] || \
        err "Must specify an output file with -o"

    if [ -f ${infile} ]; then
        case ${fmt} in
            h)
                tmp=$(mktemp -t generate-revision-dist.XXXXXX)
                generate_${fmt} ${infile} ${tmp}
                cmp -s ${tmp} ${outfile} || cp -p ${tmp} ${outfile}
                rm -f ${tmp}
                ;;
            *)
                err "Unknown format ${fmt}"
                ;;
        esac
    else
        [ -f ${outfile} ]
    fi
}

main "${@}"

# vim: syntax=sh:expandtab:shiftwidth=4:softtabstop=4
