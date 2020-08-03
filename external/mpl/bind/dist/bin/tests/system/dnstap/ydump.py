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
except (ModuleNotFoundError, ImportError):
    print("No python yaml module, skipping")
    sys.exit(1)

import subprocess
import pprint

DNSTAP_READ = sys.argv[1]
DATAFILE = sys.argv[2]
ARGS = [DNSTAP_READ, '-y', DATAFILE]

with subprocess.Popen(ARGS, stdout=subprocess.PIPE) as f:
    for y in yaml.load_all(f.stdout, Loader=yaml.SafeLoader):
        pprint.pprint(y)
