# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

rm -f named-compilezone
ln -s $CHECKZONE named-compilezone

./named-compilezone -D -F raw -o good1.db.raw example \
        zones/good1.db > /dev/null 2>&1
./named-compilezone -D -F map -o good1.db.map example \
        zones/good1.db > /dev/null 2>&1
