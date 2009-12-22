#! /bin/sh
#
# Automated Testing Framework (atf)
#
# Copyright (c) 2007, 2008, 2009 The NetBSD Foundation, Inc.
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
# Generates a header file with information about the revision used to
# build ATF.
#

set -e

Prog_Name=${0##*/}

MTN=
ROOT=

#
# err message
#
err() {
    echo "${Prog_Name}: ${@}" 1>&2
    exit 1
}

#
# call_mtn args
#
call_mtn() {
    ${MTN} --root=${ROOT} "${@}"
}

#
# get_rev_info_into_vars
#
# Sets the following variables to describe the current revision of the tree:
#    rev_base_id: The base revision identifier.
#    rev_modified: true if the tree has been locally modified.
#
get_rev_info_into_vars() {
    rev_base_id=$(call_mtn automate get_base_revision_id)

    if call_mtn status | grep "no changes" >/dev/null; then
        rev_modified=false
    else
        rev_modified=true
    fi

    datetime=$(call_mtn automate certs ${rev_base_id} | \
               grep 'value "[2-9][0-9][0-9][0-9]-[01][0-9]-[0-3][0-9]' | \
               cut -d '"' -f 2)
    rev_date=$(echo ${datetime} | cut -d T -f 1)
    rev_time=$(echo ${datetime} | cut -d T -f 2)
}

#
# generate_h revfile
#
generate_h() {
    revfile=${1}

    get_rev_info_into_vars

    >${revfile}

    echo "#define PACKAGE_REVISION_BASE \"${rev_base_id}\"" >>${revfile}

    if [ ${rev_modified} = true ]; then
        echo "#define PACKAGE_REVISION_MODIFIED 1" >>${revfile}
    fi

    echo "#define PACKAGE_REVISION_DATE \"${rev_date}\"" >>${revfile}
    echo "#define PACKAGE_REVISION_TIME \"${rev_time}\"" >>${revfile}
}

#
# main
#
# Entry point.
#
main() {
    fmt=
    outfile=
    version=
    while getopts :f:m:r:o:v: arg; do
        case ${arg} in
            f)
                fmt=${OPTARG}
                ;;
            m)
                MTN=${OPTARG}
                ;;
            o)
                outfile=${OPTARG}
                ;;
            r)
                ROOT=${OPTARG}
                ;;
            v)
                version=${OPTARG}
                ;;
            *)
                err "Unknown option ${arg}"
                ;;
        esac
    done
    [ -n "${ROOT}" ] || \
        err "Must specify the top-level source directory with -r"
    [ -n "${fmt}" ] || \
        err "Must specify an output format with -f"
    [ -n "${outfile}" ] || \
        err "Must specify an output file with -o"
    [ -n "${version}" ] || \
        err "Must specify a version number with -v"

    if [ -n "${MTN}" -a -d ${ROOT}/_MTN ]; then
        case ${fmt} in
            h)
                tmp=$(mktemp -t generate-revision.XXXXXX)
                generate_${fmt} ${tmp} ${version}
                cmp -s ${tmp} ${outfile} || cp -p ${tmp} ${outfile}
                rm -f ${tmp}
                ;;
            *)
                err "Unknown format ${fmt}"
                ;;
        esac
    else
        rm -f ${outfile}
    fi
}

main "${@}"

# vim: syntax=sh:expandtab:shiftwidth=4:softtabstop=4
