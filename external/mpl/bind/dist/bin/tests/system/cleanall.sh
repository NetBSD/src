#!/bin/sh
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

#
# Clean up after system tests.
#

SYSTEMTESTTOP=.
. $SYSTEMTESTTOP/conf.sh


find . -type f \( \
    -name '*~' -o -name 'core' -o -name '*.core' \
    -o -name '*.log' -o -name '*.pid' -o -name '*.keyset' \
    -o -name named.run -o -name ans.run \
    -o -name '*-valgrind-*.log' \) -print | xargs rm -f

status=0

rm -f $SYSTEMTESTTOP/random.data

for d in $SUBDIRS
do
   test ! -f $d/clean.sh || ( cd $d && $SHELL clean.sh )
   test -f $d/test.output && rm $d/test.output
   test -d $d && find $d -type d -exec rmdir '{}' \; 2> /dev/null
done
