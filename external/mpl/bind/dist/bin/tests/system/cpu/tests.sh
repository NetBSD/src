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

# shellcheck source=conf.sh
. ../conf.sh

status=0
n=0

CPUSET=$(command -v cpuset)
NUMACTL=$(command -v numactl)
TASKSET=$(command -v taskset)

cpulist() (
  if [ -n "$CPUSET" ]; then
    cpuset -g | head -1 | sed -e "s/.*: //" | tr -s ', ' '\n'
  elif [ -n "$NUMACTL" ]; then
    numactl --show | sed -ne 's/^physcpubind: //p' | tr -s ' ' '\n'
  elif [ -n "$TASKSET" ]; then
    # shellcheck disable=SC2046
    seq $(taskset -c -p $$ | sed -e 's/.*: //' | tr -s ' -' ' ')
  else
    echo 0
  fi
)

cpulimit() (
  set -x
  min_cpu="${1}"
  shift
  max_cpu="${1}"
  shift

  if [ -n "$CPUSET" ]; then
    cpuset -l "${min_cpu}-${max_cpu}" "$@" 2>&1
  elif [ -n "$NUMACTL" ]; then
    numactl --physcpubind="${min_cpu}-${max_cpu}" "$@" 2>&1
  elif [ -n "$TASKSET" ]; then
    taskset -c "${min_cpu}-${max_cpu}" "$@" 2>&1
  fi
)

ret=0
for cpu in $(cpulist); do
  n=$((n + 1))
  echo_i "testing that limiting CPU sets to 0-${cpu} works ($n)"
  cpulimit 0 "$cpu" "$NAMED" -g >named.run.$n 2>&1 || true
  ncpus=$(sed -ne 's/.*found \([0-9]*\) CPU.*\([0-9]*\) worker thread.*/\1/p' named.run.$n)
  [ "$ncpus" -eq "$((cpu + 1))" ] || ret=1
done
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
