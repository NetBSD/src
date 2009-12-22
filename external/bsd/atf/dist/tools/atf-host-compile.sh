#! __ATF_SHELL__
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

Prog_Name=${0##*/}
Atf_Pkgdatadir="__ATF_PKGDATADIR__"
Atf_Shell="__ATF_SHELL__"

set -e

err()
{
    echo "${Prog_Name}: ${@}" 1>&2
    exit 1
}

usage()
{
    echo "${Prog_Name}: ${@}" 1>&2
    echo "Usage: ${Prog_Name} -o outfile srcfile" 1>&2
    exit 1
}

main()
{
    [ -f "${Atf_Pkgdatadir}/atf.init.subr" ] || \
        err "Could not find ${Atf_Pkgdatadir}/atf.init.subr"

    [ ${#} -ge 1 ] || usage "No -o option specified"
    [ ${1} = '-o' ] || usage "No -o option specified"
    [ ${#} -ge 2 ] || usage "No target file specified"
    [ ${#} -ge 3 ] || usage "No source file specified"

    shift # Strip -o
    tfile=${1}; shift
    sfiles="${@}"

    for sfile in ${sfiles}; do
        [ "${tfile}" != "${sfile}" ] || \
            err "Source file and target file must not be the same"

        [ -f "${sfile}" ] || err "Source file ${sfile} does not exist"
    done

    echo "#! ${Atf_Shell}" >${tfile}
    cat ${Atf_Pkgdatadir}/atf.init.subr >>${tfile}
    echo >>${tfile}
    echo '. ${Atf_Pkgdatadir}/atf.header.subr' >>${tfile}
    echo >>${tfile}
    cat ${sfiles} >>${tfile}
    echo '. ${Atf_Pkgdatadir}/atf.footer.subr' >>${tfile}
    echo >>${tfile}
    echo "main \"\${@}\"" >>${tfile}

    chmod +x ${tfile}

    exit 0
}

main "${@}"

# vim: syntax=sh:expandtab:shiftwidth=4:softtabstop=4
