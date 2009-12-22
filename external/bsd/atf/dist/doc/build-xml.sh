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

# -------------------------------------------------------------------------
# Variables expected in the environment.
# -------------------------------------------------------------------------

: ${DOC_BUILD:=UNSET}
: ${LINKS:=UNSET}
: ${TIDY:=UNSET}
: ${XML_CATALOG_FILE:=UNSET}
: ${XMLLINT:=UNSET}
: ${XSLTPROC:=UNSET}

# -------------------------------------------------------------------------
# Global variables.
# -------------------------------------------------------------------------

Prog_Name=${0##*/}

# -------------------------------------------------------------------------
# Auxiliary functions.
# -------------------------------------------------------------------------

#
# err message
#
err() {
    for line in "${@}"; do
        echo "${Prog_Name}: ${line}" 1>&2
    done
    exit 1
}

#
# validate_xml xml_file
#
validate_xml() {
    echo "XML_CATALOG_FILES=${XML_CATALOG_FILE} ${XMLLINT}" \
         "--nonet --valid --path doc ${1}"
    XML_CATALOG_FILES=${XML_CATALOG_FILE} \
        ${XMLLINT} --nonet --valid --path doc ${1} >/dev/null
    [ ${?} -eq 0 ] || err "XML validation of ${1} failed"
}

#
# run_xsltproc input stylesheet output [args]
#
run_xsltproc() {
    input=${1}; shift
    stylesheet=${1}; shift
    output=${1}; shift
    echo "${XSLTPROC} --path doc ${*} ${stylesheet} ${input} >${output}"
    if ${XSLTPROC} --path doc "${@}" ${stylesheet} ${input} >${output}; then
        :
    else
        err "XSLT processing of ${input} with ${stylesheet} failed"
    fi
}

#
# tidy_xml input output
#
tidy_xml() {
    echo "${TIDY} -quiet -xml ${1} >${2}"
    if ${TIDY} -quiet -xml ${1} >${2}; then
        :
    else
        err "Tidying of ${1} failed"
    fi
}

#
# xml_to_html xml_input xslt_file html_output
#
xml_to_html() {
    xml_input=${1}
    xslt_file=${2}
    html_output=${3}

    tmp1=$(mktemp -t build-xml.XXXXXX)
    tmp2=$(mktemp -t build-xml.XXXXXX)

    mkdir -p ${html_output%/*}
    validate_xml ${xml_input}
    run_xsltproc ${xml_input} ${xslt_file} ${tmp1}
    tidy_xml ${tmp1} ${tmp2}

    echo "rm -f ${tmp1}"
    rm -f ${tmp1}
    echo "mv -f ${tmp2} ${html_output}"
    mv -f ${tmp2} ${html_output}
}

#
# format_txt input output
#
format_txt() {
    echo "${LINKS} -dump ${1} >${2}"
    if ${LINKS} -dump ${1} >${2}; then
        :
    else
        err "Formatting of ${1} failed"
    fi
}

#
# xml_to_txt xml_input xslt_file txt_output
#
xml_to_txt() {
    xml_input=${1}
    xslt_file=${2}
    txt_output=${3}

    tmp1=$(mktemp -t build-xml.XXXXXX)
    tmp2=$(mktemp -t build-xml.XXXXXX)

    mkdir -p ${txt_output%/*}
    validate_xml ${xml_input}
    run_xsltproc ${xml_input} ${xslt_file} ${tmp1}
    format_txt ${tmp1} ${tmp2}

    echo "rm -f ${tmp1}"
    rm -f ${tmp1}
    echo "mv -f ${tmp2} ${txt_output}"
    mv -f ${tmp2} ${txt_output}
}

#
# xml_to_anything xml_input xslt_file output
#
xml_to_anything() {
    xml_input=${1}
    xslt_file=${2}
    format=$(echo ${3} | cut -d : -f 1)
    output=$(echo ${3} | cut -d : -f 2)

    case ${format} in
        txt|html)
            xml_to_${format} ${xml_input} ${xslt_file} ${output}
            ;;
        *)
            err "Unknown format ${format}"
            ;;
    esac
}

# -------------------------------------------------------------------------
# Main program.
# -------------------------------------------------------------------------

main() {
    [ ${#} -eq 3 ] || usage

    xml_input=${1}
    xslt_file=${2}
    output=${3}

    if [ ${DOC_BUILD} = yes ]; then
        xml_to_anything ${1} ${2} ${3}
        true
    else
        err "Cannot regenerate ${output}" \
            "Reconfigure the package using --enable-doc-build"
    fi
}

usage() {
    cat 1>&2 <<EOF
Usage: ${Prog_Name} <xml-input> <xslt-file> <format>:<output>
EOF
    exit 1
}

main "${@}"

# vim: syntax=sh:expandtab:shiftwidth=4:softtabstop=4
