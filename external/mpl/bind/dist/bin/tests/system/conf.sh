#!/bin/sh
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

# When sourcing the script outside the pytest environment (e.g. during helper
# script development), the env variables have to be loaded.
if [ -z "$TOP_SRCDIR" ]; then
  SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd | sed -E 's|(.*bin/tests/system).*|\1|')
  eval "$(PYTHONPATH="$SCRIPT_DIR:$PYTHONPATH" /usr/bin/env python3 -m isctest)"
fi

testsock6() {
  if test -n "$PERL" && $PERL -e "use IO::Socket::IP;" 2>/dev/null; then
    $PERL "$TOP_SRCDIR/bin/tests/system/testsock6.pl" "$@"
  else
    false
  fi
}

echofail() {
  echo "$*"
}
echowarn() {
  echo "$*"
}
echopass() {
  echo "$*"
}
echoinfo() {
  echo "$*"
}
echostart() {
  echo "$*"
}
echoend() {
  echo "$*"
}

echo_i() {
  echo "$@" | while IFS= read -r __LINE; do
    echoinfo "I:$__LINE"
  done
}

echo_ic() {
  echo "$@" | while IFS= read -r __LINE; do
    echoinfo "I:  $__LINE"
  done
}

echo_d() {
  echo "$@" | while IFS= read -r __LINE; do
    echoinfo "D:$__LINE"
  done
}

cat_i() {
  while IFS= read -r __LINE; do
    echoinfo "I:$__LINE"
  done
}

cat_d() {
  while IFS= read -r __LINE; do
    echoinfo "D:$__LINE"
  done
}

digcomp() {
  {
    output=$($PERL $TOP_SRCDIR/bin/tests/system/digcomp.pl "$@")
    result=$?
  } || true
  [ -n "$output" ] && {
    echo "digcomp failed:"
    echo "$output"
  } | cat_i
  return $result
}

start_server() {
  $PERL "$TOP_SRCDIR/bin/tests/system/start.pl" "$SYSTESTDIR" "$@"
}

stop_server() {
  $PERL "$TOP_SRCDIR/bin/tests/system/stop.pl" "$SYSTESTDIR" "$@"
}

send() {
  $PERL "$TOP_SRCDIR/bin/tests/system/send.pl" "$@"
}

#
# Useful functions in test scripts
#

# assert_int_equal: compare two integer variables, $1 and $2
#
# If $1 and $2 are equal, return 0; if $1 and $2 are not equal, report
# the error using the description of the tested variable provided in $3
# and return 1.
assert_int_equal() {
  found="$1"
  expected="$2"
  description="$3"

  if [ "${expected}" -ne "${found}" ]; then
    echo_i "incorrect ${description}: got ${found}, expected ${expected}"
    return 1
  fi

  return 0
}

# keyfile_to_keys_section: helper function for keyfile_to_*_keys() which
# converts keyfile data into a key-style trust anchor configuration
# section using the supplied parameters
keyfile_to_keys() {
  section_name=$1
  key_prefix=$2
  shift
  shift
  echo "$section_name {"
  for keyname in $*; do
    awk '!/^; /{
	    printf "\t\""$1"\" "
	    printf "'"$key_prefix "'"
	    printf $4 " " $5 " " $6 " \""
	    for (i=7; i<=NF; i++) printf $i
	    printf "\";\n"
	}' $keyname.key
  done
  echo "};"
}

# keyfile_to_dskeys_section: helper function for keyfile_to_*_dskeys()
# converts keyfile data into a DS-style trust anchor configuration
# section using the supplied parameters
keyfile_to_dskeys() {
  section_name=$1
  key_prefix=$2
  shift
  shift
  echo "$section_name {"
  for keyname in $*; do
    $DSFROMKEY $keyname.key \
      | awk '!/^; /{
	    printf "\t\""$1"\" "
	    printf "'"$key_prefix "'"
	    printf $4 " " $5 " " $6 " \""
	    for (i=7; i<=NF; i++) printf $i
	    printf "\";\n"
	}'
  done
  echo "};"
}

# keyfile_to_trusted_keys: convert key data contained in the keyfile(s)
# provided to a "trust-keys" section suitable for including in a
# resolver's configuration file
keyfile_to_trusted_keys() {
  keyfile_to_keys "trusted-keys" "" $*
}

# keyfile_to_static_keys: convert key data contained in the keyfile(s)
# provided to a *static-key* "trust-anchors" section suitable for including in
# a resolver's configuration file
keyfile_to_static_keys() {
  keyfile_to_keys "trust-anchors" "static-key" $*
}

# keyfile_to_initial_keys: convert key data contained in the keyfile(s)
# provided to an *initial-key* "trust-anchors" section suitable for including
# in a resolver's configuration file
keyfile_to_initial_keys() {
  keyfile_to_keys "trust-anchors" "initial-key" $*
}

# keyfile_to_static_ds_keys: convert key data contained in the keyfile(s)
# provided to a *static-ds* "trust-anchors" section suitable for including in a
# resolver's configuration file
keyfile_to_static_ds() {
  keyfile_to_dskeys "trust-anchors" "static-ds" $*
}

# keyfile_to_initial_ds_keys: convert key data contained in the keyfile(s)
# provided to an *initial-ds* "trust-anchors" section suitable for including
# in a resolver's configuration file
keyfile_to_initial_ds() {
  keyfile_to_dskeys "trust-anchors" "initial-ds" $*
}

# keyfile_to_key_id: convert a key file name to a key ID
#
# For a given key file name (e.g. "Kexample.+013+06160") provided as $1,
# print the key ID with leading zeros stripped ("6160" for the
# aforementioned example).
keyfile_to_key_id() {
  echo "$1" | sed "s/.*+0\{0,4\}//"
}

# private_type_record: write a private type record recording the state of the
# signing process
#
# For a given zone ($1), algorithm number ($2) and key file ($3), print the
# private type record with default type value of 65534, indicating that the
# signing process for this key is completed.
private_type_record() {
  _zone=$1
  _algorithm=$2
  _keyfile=$3

  _id=$(keyfile_to_key_id "$_keyfile")

  printf "%s. 0 IN TYPE65534 %s 5 %02x%04x0000\n" "$_zone" "\\#" "$_algorithm" "$_id"
}

# nextpart*() - functions for reading files incrementally
#
# These functions aim to facilitate looking for (or waiting for)
# messages which may be logged more than once throughout the lifetime of
# a given named instance by outputting just the part of the file which
# has been appended since the last time we read it.
#
# Calling some of these functions causes temporary *.prev files to be
# created.
#
# Note that unlike other nextpart*() functions, nextpartread() is not
# meant to be directly used in system tests; its sole purpose is to
# reduce code duplication below.
#
# A quick usage example:
#
#     $ echo line1 > named.log
#     $ echo line2 >> named.log
#     $ nextpart named.log
#     line1
#     line2
#     $ echo line3 >> named.log
#     $ nextpart named.log
#     line3
#     $ nextpart named.log
#     $ echo line4 >> named.log
#     $ nextpartpeek named.log
#     line4
#     $ nextpartpeek named.log
#     line4
#     $ nextpartreset named.log
#     $ nextpartpeek named.log
#     line1
#     line2
#     line3
#     line4
#     $ nextpart named.log
#     line1
#     line2
#     line3
#     line4
#     $ nextpart named.log
#     $

# nextpartreset: reset the marker used by nextpart() and nextpartpeek()
# so that it points to the start of the given file
nextpartreset() {
  echo "0" >$1.prev
}

# nextpartread: read everything that's been appended to a file since the
# last time nextpart() was called and print it to stdout, print the
# total number of lines read from that file so far to file descriptor 3
nextpartread() {
  [ -f $1.prev ] || nextpartreset $1
  prev=$(cat $1.prev)
  awk "NR > $prev "'{ print }
	 END          { print NR > "/dev/stderr" }' $1 2>&3
}

# nextpart: read everything that's been appended to a file since the
# last time nextpart() was called
nextpart() {
  nextpartread $1 3>$1.prev.tmp
  mv $1.prev.tmp $1.prev
}

# nextpartpeek: read everything that's been appended to a file since the
# last time nextpart() was called
nextpartpeek() {
  nextpartread $1 3>/dev/null
}

# _search_log: look for message $1 in file $2 with nextpart().
_search_log() (
  msg="$1"
  file="$2"
  nextpart "$file" | grep -F -e "$msg" >/dev/null
)

# _search_log_re: same as _search_log but the message is an grep -E regex
_search_log_re() (
  msg="$1"
  file="$2"
  nextpart "$file" | grep -E -e "$msg" >/dev/null
)

# _search_log_peek: look for message $1 in file $2 with nextpartpeek().
_search_log_peek() (
  msg="$1"
  file="$2"
  nextpartpeek "$file" | grep -F -e "$msg" >/dev/null
)

# wait_for_log: wait until message $2 in file $3 appears.  Bail out after
# $1 seconds.  This needs to be used in conjunction with a prior call to
# nextpart() or nextpartreset() on the same file to guarantee the offset is
# set correctly.  Tests using wait_for_log() are responsible for cleaning up
# the created <file>.prev files.
wait_for_log() (
  timeout="$1"
  msg="$2"
  file="$3"
  retry_quiet "$timeout" _search_log "$msg" "$file" && return 0
  echo_i "exceeded time limit waiting for literal '$msg' in $file"
  return 1
)

# wait_for_log_re: same as wait_for_log, but the message is an grep -E regex
wait_for_log_re() (
  timeout="$1"
  msg="$2"
  file="$3"
  retry_quiet "$timeout" _search_log_re "$msg" "$file" && return 0
  echo_i "exceeded time limit waiting for regex '$msg' in $file"
  return 1
)

# wait_for_log_peek: similar to wait_for_log() but peeking, so the file offset
# does not change.
wait_for_log_peek() (
  timeout="$1"
  msg="$2"
  file="$3"
  retry_quiet "$timeout" _search_log_peek "$msg" "$file" && return 0
  echo_i "exceeded time limit waiting for literal '$msg' in $file"
  return 1
)

# _retry: keep running a command until it succeeds, up to $1 times, with
# one-second intervals, optionally printing a message upon every attempt
_retry() {
  __retries="${1}"
  shift

  while :; do
    if "$@"; then
      return 0
    fi
    __retries=$((__retries - 1))
    if [ "${__retries}" -gt 0 ]; then
      if [ "${__retry_quiet}" -ne 1 ]; then
        echo_i "retrying"
      fi
      sleep 1
    else
      return 1
    fi
  done
}

# retry: call _retry() in verbose mode
retry() {
  __retry_quiet=0
  _retry "$@"
}

# retry_quiet: call _retry() in silent mode
retry_quiet() {
  __retry_quiet=1
  _retry "$@"
}

# _repeat: keep running command up to $1 times, unless it fails
_repeat() (
  __retries="${1}"
  shift
  while :; do
    if ! "$@"; then
      return 1
    fi
    __retries=$((__retries - 1))
    if [ "${__retries}" -le 0 ]; then
      break
    fi
  done
  return 0
)

_times() {
  awk "BEGIN{ for(i = 1; i <= $1; i++) print i}"
}

rndc_reload() {
  $RNDC -c ../_common/rndc.conf -s $2 -p ${CONTROLPORT} reload $3 2>&1 | sed 's/^/'"I:$1"' /'
  # reloading single zone is synchronous, if we're reloading whole server
  # we need to wait for reload to finish
  if [ -z "$3" ]; then
    for _ in $(_times 10); do
      $RNDC -c ../_common/rndc.conf -s $2 -p ${CONTROLPORT} status | grep "reload/reconfig in progress" >/dev/null || break
      sleep 1
    done
  fi
}

rndc_reconfig() {
  seconds=${3:-10}
  $RNDC -c ../_common/rndc.conf -s "$2" -p "${CONTROLPORT}" reconfig 2>&1 | sed 's/^/'"I:$1"' /'
  for _ in $(_times "$seconds"); do
    "$RNDC" -c ../_common/rndc.conf -s "$2" -p "${CONTROLPORT}" status | grep "reload/reconfig in progress" >/dev/null || break
    sleep 1
  done
}

# rndc_dumpdb: call "rndc dumpdb [...]" and wait until it completes
#
# The first argument is the name server instance to send the command to, in the
# form of "nsX" (where "X" is the instance number), e.g. "ns5".  The remaining
# arguments, if any, are appended to the rndc command line after "dumpdb".
#
# Control channel configuration for the name server instance to send the
# command to must match the contents of bin/tests/system/_common/rndc.conf.
#
# rndc output is stored in a file called rndc.out.test${n}; the "n" variable is
# required to be set by the calling tests.sh script.
#
# Return 0 if the dump completes successfully; return 1 if rndc returns an exit
# code other than 0 or if the "; Dump complete" string does not appear in the
# dump within 10 seconds.
rndc_dumpdb() {
  __ret=0
  __dump_complete=0
  __server="${1}"
  __ip="10.53.0.$(echo "${__server}" | tr -c -d '[:digit:]')"

  shift
  ${RNDC} -c ../_common/rndc.conf -p "${CONTROLPORT}" -s "${__ip}" dumpdb "$@" >"rndc.out.test${n}" 2>&1 || __ret=1

  for _ in 0 1 2 3 4 5 6 7 8 9; do
    if grep '^; Dump complete$' "${__server}/named_dump.db" >/dev/null; then
      mv "${__server}/named_dump.db" "${__server}/named_dump.db.test${n}"
      __dump_complete=1
      break
    fi
    sleep 1
  done

  if [ ${__dump_complete} -eq 0 ]; then
    echo_i "timed out waiting for 'rndc dumpdb' to finish"
    __ret=1
  fi

  return ${__ret}
}

# get_dig_xfer_stats: extract transfer statistics from dig output stored
# in $1, converting them to a format used by some system tests.
get_dig_xfer_stats() {
  LOGFILE="$1"
  sed -n "s/^;; XFR size: .*messages \([0-9][0-9]*\).*/messages=\1/p" "${LOGFILE}"
  sed -n "s/^;; XFR size: \([0-9][0-9]*\) records.*/records=\1/p" "${LOGFILE}"
  sed -n "s/^;; XFR size: .*bytes \([0-9][0-9]*\).*/bytes=\1/p" "${LOGFILE}"
}

# get_named_xfer_stats: from named log file $1, extract transfer
# statistics for the last transfer for peer $2 and zone $3 (from a log
# message which has to contain the string provided in $4), converting
# them to a format used by some system tests.
get_named_xfer_stats() {
  LOGFILE="$1"
  PEER="$(echo $2 | sed 's/\./\\./g')"
  ZONE="$(echo $3 | sed 's/\./\\./g')"
  MESSAGE="$4"
  grep " ${PEER}#.*${MESSAGE}:" "${LOGFILE}" \
    | sed -n "s/.* '${ZONE}\/.* \([0-9][0-9]*\) messages.*/messages=\1/p" | tail -1
  grep " ${PEER}#.*${MESSAGE}:" "${LOGFILE}" \
    | sed -n "s/.* '${ZONE}\/.* \([0-9][0-9]*\) records.*/records=\1/p" | tail -1
  grep " ${PEER}#.*${MESSAGE}:" "${LOGFILE}" \
    | sed -n "s/.* '${ZONE}\/.* \([0-9][0-9]*\) bytes.*/bytes=\1/p" | tail -1
}

grep_v() { grep -v "$@" || test $? = 1; }
