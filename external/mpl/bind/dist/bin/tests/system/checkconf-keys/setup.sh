#!/bin/sh -e

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

set -e

mkdir ksk
mkdir zsk

zone="default.example"
cp template.db.in "${zone}.db"
$KEYGEN -a 13 -fK $zone 2>keygen.out.$zone.1

zone="bad-default-kz.example"
cp template.db.in "${zone}.db"
$KEYGEN -a 13 -fK $zone 2>keygen.out.$zone.1
$KEYGEN -a 13 $zone 2>keygen.out.$zone.2

zone="bad-default-algorithm.example"
cp template.db.in "${zone}.db"
$KEYGEN -a 8 -fK $zone 2>keygen.out.$zone.1

zone="alternative.kz.example"
cp template.db.in "${zone}.db"
$KEYGEN -a RSASHA256 -b 2048 $zone 2>keygen.out.$zone.1
$KEYGEN -a RSASHA256 -b 2048 -fK $zone 2>keygen.out.$zone.2

zone="alternative.csk.example"
cp template.db.in "${zone}.db"
$KEYGEN -a RSASHA256 -b 2048 -fK $zone 2>keygen.out.$zone.2

zone="default.kz.example"
cp template.db.in "${zone}.db"
$KEYGEN -a 13 $zone 2>keygen.out.$zone.1
$KEYGEN -a 13 -fK $zone 2>keygen.out.$zone.2

zone="default.csk.example"
cp template.db.in "${zone}.db"
$KEYGEN -a 13 -fK $zone 2>keygen.out.$zone.2

zone="keystores.kz.example"
cp template.db.in "${zone}.db"
$KEYGEN -a 13 -fK -K ksk $zone 2>keygen.out.$zone.2
$KEYGEN -a 13 -K zsk $zone 2>keygen.out.$zone.2

zone="superfluous-keyfile.kz.example"
cp template.db.in "${zone}.db"
$KEYGEN -a 13 $zone 2>keygen.out.$zone.1
$KEYGEN -a 13 -fK $zone 2>keygen.out.$zone.2
$KEYGEN -a 13 $zone 2>keygen.out.$zone.3 # superfluous

zone="missing-keyfile.kz.example"
cp template.db.in "${zone}.db"
$KEYGEN -a 13 $zone 2>keygen.out.$zone.1
# no ksk

zone="bad-algorithm.kz.example"
cp template.db.in "${zone}.db"
$KEYGEN -a 13 $zone 2>keygen.out.$zone.1
$KEYGEN -a 13 -fK $zone 2>keygen.out.$zone.2

zone="bad-length.csk.example"
cp template.db.in "${zone}.db"
$KEYGEN -a 8 -b 4096 -fK $zone 2>keygen.out.$zone.2

zone="bad-tagrange.csk.example"
cp template.db.in "${zone}.db"
$KEYGEN -a 13 -M 32768:65535 -fK $zone 2>keygen.out.$zone.2

zone="bad-role.kz.example"
cp template.db.in "${zone}.db"
$KEYGEN -a 13 -fK $zone 2>keygen.out.$zone.1
$KEYGEN -a 13 -fK $zone 2>keygen.out.$zone.2
