############################################################################
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.
############################################################################

import sys

try:
    import yaml
# flake8: noqa: E722
# pylint: disable=bare-except
except:
    print("No python yaml module, skipping")
    sys.exit(1)

with open(sys.argv[1], "r") as f:
    for item in yaml.safe_load_all(f):
        for key in sys.argv[2:]:
            try:
                key = int(key)
            except ValueError:
                pass

            try:
                item = item[key]
            except KeyError:
                print('Key "' + key + '" not found.')
                sys.exit(1)

        print(item)
