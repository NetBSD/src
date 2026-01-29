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

. ../conf.sh

$SHELL ${TOP_SRCDIR}/bin/tests/system/genzone.sh 2 >ns1/example.db

echo '; huge answer' >>ns1/example.db
x=1
while [ $x -le 50 ]; do
  y=1
  while [ $y -le 50 ]; do
    printf 'biganswer\t\tA\t\t10.10.%d.%d\n' $x $y >>ns1/example.db
    y=$((y + 1))
  done
  x=$((x + 1))
done
