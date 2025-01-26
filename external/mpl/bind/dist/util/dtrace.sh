#!/bin/sh

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0.  If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

USAGE="# Usage: ${0} [-h | -G] -s File.d [-o <File>]"

mode=
while getopts hGs:o: opt; do
  case "${opt}" in
    h) mode=header ;;
    s) input=$OPTARG ;;
    o) output=$OPTARG ;;
    G) mode=object ;;
    \?)
      echo $USAGE
      exit 1
      ;;
  esac
done
shift $((OPTIND - 1))

if test -z "${mode}" || test -z "${input}"; then
  echo $USAGE
  exit 1
fi

case "${mode}" in
  header)
    if test -z "${output}"; then
      output="$(basename "${input}" .d).h"
    fi
    PROVIDER=$(cat "${input}" | sed -ne 's/^provider \(.*\) {/\1/p' | tr "a-z" "A-Z")
    sed -ne 's/.*probe \(.*\)(.*);/\1/p' "${input}" | tr "a-z" "A-Z" | while read PROBE; do
      echo "#define ${PROVIDER}_${PROBE}_ENABLED() 0"
      echo "#define ${PROVIDER}_${PROBE}(...)"
    done >"${output}"
    ;;
  object)
    if test -z "${output}"; then
      output="$(basename "${input}" .d).o"
    fi
    echo "extern int empty;" | gcc -xc -c - -fPIC -DPIC -o "${output}"
    ;;
esac
